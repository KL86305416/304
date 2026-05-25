#include "line_follower.h"

#include "gray_sensor.h"
#include "line_follower_config.h"
#include "tb6612.h"

/*
 * 循迹核心思路：
 * 1. 八路灰度传感器先被换算成一个偏差 flag。
 * 2. flag = 0 表示黑线在车体中心；flag > 0 表示黑线偏左；flag < 0 表示黑线偏右。
 * 3. PID 根据 flag 算出差速修正量 output。
 * 4. 左右轮使用“基础速度一加一减”的方式转向。
 * 5. 如果八路全白，说明黑线离开传感器范围，按上一次黑线方向原地找线。
 */
#define LINE_LEFT_MOTOR   TB6612_MOTOR_A
#define LINE_RIGHT_MOTOR  TB6612_MOTOR_B

typedef struct
{
  float setpoint;     /* 目标偏差。循迹目标永远是 0，也就是让黑线回到中间。 */
  float kp;           /* 比例系数：偏得越多，修得越狠。 */
  float ki;           /* 积分系数：修长期偏差。当前建议保持 0，避免积分累积导致冲出。 */
  float kd;           /* 微分系数：抑制摆动，让回正动作更稳。 */
  float error;        /* 当前误差，等于 setpoint - flag。 */
  float last_error;   /* 上一次误差，用来计算误差变化速度。 */
  float error_sum;    /* 历史误差累加，给积分项使用。 */
  float output;       /* PID 输出，也就是左右轮差速修正量。 */
} LineFollower_Pid_t;

static LineFollower_Pid_t line_pid;
static float last_valid_flag = 0.0f;         /* 最近一次有效看到黑线时的 flag，OLED 丢线显示用。 */
static int8_t last_line_side = 0;            /* 黑线最后出现在哪边：1 左侧，-1 右侧，0 未知。 */
static uint8_t lost_recovery_active = 0U;    /* 丢线找回状态标志，给 OLED 显示和 PID 重置使用。 */
static LineFollower_Status_t last_status = {0}; /* 最近一次巡线状态，返回给主循环/OLED。 */

static float LineFollower_AbsFloat(float value)
{
  return (value < 0.0f) ? -value : value;
}

static int16_t LineFollower_ClampOutput(float value)
{
  /* TB6612_SetSpeed 的有效占空比范围是 0~1000，这里限制最终电机命令。 */
  if (value > (float)LINE_FOLLOWER_MAX_OUTPUT_DUTY)
  {
    return (int16_t)LINE_FOLLOWER_MAX_OUTPUT_DUTY;
  }

  if (value < 0.0f)
  {
    return 0;
  }

  return (int16_t)value;
}

static void LineFollower_SetMotorSpeed(int16_t left_speed, int16_t right_speed)
{
  /* 这里做一层封装，后续如果左右电机接反，只需要在这里调整映射。 */
  TB6612_SetSpeed(LINE_LEFT_MOTOR, left_speed);
  TB6612_SetSpeed(LINE_RIGHT_MOTOR, right_speed);
}

static float LineFollower_ReadFlag(const GraySensor_Data_t *gray_data,
                                   uint8_t *active_count)
{
  /*
   * 权重从左到右依次为正到负：
   * 左侧压线时 flag 为正，PID 输出会让左轮变慢、右轮变快，从而向左修正。
   * 右侧压线时 flag 为负，PID 输出会让左轮变快、右轮变慢，从而向右修正。
   */
  static const float sensor_weight[GRAY_SENSOR_COUNT] = {
    7.0f, 5.0f, 3.0f, 0.5f, -1.5f, -3.0f, -5.0f, 7.0f
  };
  uint8_t count = 0U;
  float weighted_sum = 0.0f;

  for (uint8_t i = 0U; i < GRAY_SENSOR_COUNT; i++)
  {
    if (gray_data->sensor[i] != 0U)
    {
      /* 多个探头同时看到黑线时取平均位置，比单点判断更平滑。 */
      weighted_sum += sensor_weight[i];
      count++;
    }
  }

  if (active_count != 0)
  {
    *active_count = count;
  }

  if ((count == 0U) || (count == GRAY_SENSOR_COUNT))
  {
    /*
     * count == 0：八路全白，真正的丢线，外层会进入找线。
     * count == 8：八路全黑，常见于起点/横线/特殊标记，这里先当作居中处理。
     */
    return 0.0f;
  }

  return weighted_sum / (float)count;
}

static void LineFollower_ResetPid(void)
{
  /* 重新进入巡线或刚找回黑线时清空 PID 记忆，避免旧误差继续影响车辆。 */
  line_pid.setpoint = 0.0f;
  line_pid.kp = LINE_FOLLOWER_PID_KP;
  line_pid.ki = LINE_FOLLOWER_PID_KI;
  line_pid.kd = LINE_FOLLOWER_PID_KD;
  line_pid.error = 0.0f;
  line_pid.last_error = 0.0f;
  line_pid.error_sum = 0.0f;
  line_pid.output = 0.0f;
}

static void LineFollower_CalcPid(float flag)
{
  /*
   * 这是位置式 PID：
   * P：当前偏差，决定转向力度。
   * I：历史偏差累加，当前默认关闭。
   * D：本次偏差和上次偏差的差，负责压住过冲和摆动。
   */
  line_pid.error = line_pid.setpoint - flag;
  line_pid.error_sum += line_pid.error;
  line_pid.output =
    (line_pid.kp * line_pid.error) +
    (line_pid.ki * line_pid.error_sum) +
    (line_pid.kd * (line_pid.error - line_pid.last_error));
  line_pid.last_error = line_pid.error;
}

static void LineFollower_UpdateLineSide(float flag)
{
  /* 只在偏差超过阈值时更新方向记忆，避免中心附近的小抖动污染丢线方向。 */
  if (flag > LINE_FOLLOWER_LINE_SIDE_MEMORY_THRESHOLD)
  {
    last_line_side = 1;
  }
  else if (flag < -LINE_FOLLOWER_LINE_SIDE_MEMORY_THRESHOLD)
  {
    last_line_side = -1;
  }
}

static void LineFollower_RecoverLostLine(int16_t *left_speed, int16_t *right_speed)
{
  /*
   * 丢线时不直接停车，而是参考“最后一次黑线在哪边”主动找回。
   * 原工程能过大弯的关键就在这里：小偏差交给 PID，大偏差/丢线交给找线动作。
   */
  lost_recovery_active = 1U;
  line_pid.error_sum = 0.0f;

  if (last_line_side > 0)
  {
    /* 黑线最后在左边：左轮停、右轮转，小车向左大转找线。 */
    *left_speed = 0;
    *right_speed = LINE_FOLLOWER_LOST_TURN_DUTY;
  }
  else if (last_line_side < 0)
  {
    /* 黑线最后在右边：右轮停、左轮转，小车向右大转找线。 */
    *left_speed = LINE_FOLLOWER_LOST_TURN_DUTY;
    *right_speed = 0;
  }
  else
  {
    *left_speed = 0;
    *right_speed = 0;
  }
}

void LineFollower_Init(void)
{
  /* 每次进入巡线模式都从干净状态开始，避免上一轮调试残留状态。 */
  LineFollower_ResetPid();

  last_valid_flag = 0.0f;
  last_line_side = 0;
  lost_recovery_active = 0U;
  last_status.raw_value = 0U;
  last_status.active_count = 0U;
  last_status.line_lost = 1U;
  last_status.curve_active = 0U;
  last_status.error_x100 = 0;
  last_status.left_speed = 0;
  last_status.right_speed = 0;

  LineFollower_Stop();
}

LineFollower_Status_t LineFollower_Update(void)
{
  /*
   * 主循环每隔 LINE_FOLLOWER_CONTROL_PERIOD_MS 调用一次本函数。
   * 一次调用完成：读传感器 -> 算 flag -> 算 PID/找线 -> 输出电机 -> 返回 OLED 状态。
   */
  GraySensor_Data_t gray_data = GraySensor_ReadDetail();
  uint8_t active_count = 0U;
  float flag = LineFollower_ReadFlag(&gray_data, &active_count);
  int16_t left_speed = 0;
  int16_t right_speed = 0;

  last_status.raw_value = gray_data.value;
  last_status.active_count = active_count;

  if (active_count == 0U)
  {
    /* 八路全白：传感器范围内没有黑线，进入丢线找回。 */
    LineFollower_RecoverLostLine(&left_speed, &right_speed);

    last_status.line_lost = 1U;
    last_status.curve_active = lost_recovery_active;
    last_status.error_x100 = (int16_t)(last_valid_flag * 100.0f);
  }
  else
  {
    if (lost_recovery_active != 0U)
    {
      /* 刚从丢线状态重新看到黑线：清空 PID 积累，防止找回瞬间猛甩。 */
      lost_recovery_active = 0U;
      LineFollower_ResetPid();
      line_pid.last_error = line_pid.setpoint - flag;
    }

    LineFollower_UpdateLineSide(flag);
    LineFollower_CalcPid(flag);

    /*
     * 单环差速输出：
     * output 为正时左轮加速、右轮减速；output 为负时相反。
     * 因为 error = 0 - flag，所以左侧黑线会得到负 output，车辆向左修正。
     */
    left_speed =
      LineFollower_ClampOutput((float)LINE_FOLLOWER_BASE_SPEED_DUTY + line_pid.output);
    right_speed =
      LineFollower_ClampOutput((float)LINE_FOLLOWER_BASE_SPEED_DUTY - line_pid.output);

    last_valid_flag = flag;
    last_status.line_lost = 0U;
    /* curve_active 这里不再代表复杂弯道状态，只作为“大偏差”提示显示。 */
    last_status.curve_active =
      (LineFollower_AbsFloat(flag) >= LINE_FOLLOWER_BIG_TURN_FLAG) ? 1U : 0U;
    last_status.error_x100 = (int16_t)(flag * 100.0f);
  }

  LineFollower_SetMotorSpeed(left_speed, right_speed);

  last_status.left_speed = left_speed;
  last_status.right_speed = right_speed;

  return last_status;
}

void LineFollower_Stop(void)
{
  /* 退出巡线页面或初始化时停止两个电机。 */
  LineFollower_SetMotorSpeed(0, 0);
}
