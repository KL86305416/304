#ifndef __LINE_FOLLOWER_CONFIG_H__
#define __LINE_FOLLOWER_CONFIG_H__

/*
 * 巡线算法配置：
 * 1. 八路灰度数据会先换算成 flag，左侧黑线为正，右侧黑线为负。
 * 2. PID 只做单环差速，左轮 = 基础速度 + 输出，右轮 = 基础速度 - 输出。
 * 3. 八路全白时不停车，按上一次黑线方向大转找线，重新检测到黑线后恢复 PID。
 * 4. 速度单位为 TB6612 PWM 占空比，范围 0~1000。
 */
#define LINE_FOLLOWER_BASE_SPEED_DUTY             180
#define LINE_FOLLOWER_MAX_OUTPUT_DUTY             1000

#define LINE_FOLLOWER_PID_KP                      24.0f
#define LINE_FOLLOWER_PID_KI                      0.0f
#define LINE_FOLLOWER_PID_KD                      9.0f

#define LINE_FOLLOWER_LOST_TURN_DUTY              420
#define LINE_FOLLOWER_LINE_SIDE_MEMORY_THRESHOLD  0.5f
#define LINE_FOLLOWER_BIG_TURN_FLAG               2.0f

#define LINE_FOLLOWER_CONTROL_PERIOD_MS           10U
#define LINE_FOLLOWER_OLED_UPDATE_MS              300U

#endif /* __LINE_FOLLOWER_CONFIG_H__ */
