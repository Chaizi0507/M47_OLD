/**
 * @file ita_chariot.cpp
 * @author yssickjgd (yssickjgd@mail.ustc.edu.cn)
 * @brief 人机交互控制逻辑
 * @version 0.1
 * @date 2023-08-29 0.1 23赛季定稿
 *
 * @copyright USTC-RoboWalker (c) 2022
 *
 */

/* Includes ------------------------------------------------------------------*/

#include "ita_chariot.h"
#include "drv_math.h"
/* Private macros ------------------------------------------------------------*/

/* Private types -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Private function declarations ---------------------------------------------*/

/* Function prototypes -------------------------------------------------------*/

/**
 * @brief 控制交互端初始化
 *
 */
void Class_Chariot::Init(float __DR16_Dead_Zone)
{
#ifdef CHASSIS

    // 裁判系统
    Referee.Init(&huart6);

    // 底盘
    Chassis.Referee = &Referee;
    Chassis.Init(10.0f, 10.0f, 2.0f);

    // 底盘随动PID环初始化
    PID_Chassis_Fllow.Init(20.0f, 0.0f, 0.0f, 0.0f, 20.0f, 20.0f, 0.0f, 0.0f, 0.0f, 0.001f, 0.05f);

#ifdef POWER_LIMIT
    // 串口超电
    Supercap.Init_UART(&huart1);
#endif

#elif defined(GIMBAL)

    // 上位机
    MiniPC.Init(&MiniPC_USB_Manage_Object);
    MiniPC.IMU = &Gimbal.Boardc_BMI;
    MiniPC.Referee = &Referee;
    MiniPC.Gimbal_Pitch_Motor_GM6020 = &Gimbal.Motor_Pitch;
    MiniPC.Gimbal_Yaw_Motor_GM6020 = &Gimbal.Motor_Yaw;
    Chassis.Set_Velocity_X_Max(0.9f);
    Chassis.Set_Velocity_Y_Max(0.9f);

    // 遥控器
    DR16.Init(&huart3);
    DR16_Dead_Zone = __DR16_Dead_Zone;

    // 云台
    Gimbal.Init();

    // 发射机构
    Booster.Referee = &Referee;
    Booster.Gimbal = &Gimbal;
    Booster.Init();



    Chassis.Chassis_Follow_PID_Angle.Init(0.15f, 0.0f, 0.0f, 0.0f, 200.0f, 200.0f);
    // select the MiniPC object
    Gimbal.MiniPC = &MiniPC;
   
#endif
}

/**
 * @brief can回调函数处理云台发来的数据
 *
 */
#ifdef CHASSIS
void Class_Chariot::CAN_Chassis_Rx_Gimbal_Callback()
{
    // 云台坐标系的目标速度
    float gimbal_velocity_x, gimbal_velocity_y;
    // 底盘坐标系的目标速度
    float chassis_velocity_x, chassis_velocity_y;
    // 底盘和云台夹角（弧度制）
    float derta_angle;
    // float映射到int16之后的速度
    uint16_t tmp_velocity_x, tmp_velocity_y;
    // 底盘控制类型
    Enum_Chassis_Control_Type tmp_chassis_control_type;
    // 底盘角速度
    float target_omega;

    memcpy(&tmp_velocity_x, &CAN_Manage_Object->Rx_Buffer.Data[0], sizeof(int16_t));
    memcpy(&tmp_velocity_y, &CAN_Manage_Object->Rx_Buffer.Data[2], sizeof(int16_t));
    memcpy(&tmp_chassis_control_type, &CAN_Manage_Object->Rx_Buffer.Data[4], sizeof(uint8_t));
    // Math_Endian_Reverse_16((void *)CAN_Manage_Object->Rx_Buffer.Data[0], (void *)&tmp_velocity_x);
    // Math_Endian_Reverse_16((void *)CAN_Manage_Object->Rx_Buffer.Data[2], (void *)&tmp_velocity_y);

    gimbal_velocity_x = Math_Int_To_Float(tmp_velocity_x, 0, 0x7FFF, -1 * Chassis.Get_Velocity_X_Max(), Chassis.Get_Velocity_X_Max());
    gimbal_velocity_y = Math_Int_To_Float(tmp_velocity_y, 0, 0x7FFF, -1 * Chassis.Get_Velocity_Y_Max(), Chassis.Get_Velocity_Y_Max());

    // 获取云台坐标系和底盘坐标系的夹角（弧度制）
    Chassis_Angle = Chassis.Motor_Yaw.Get_Now_Angle();
    derta_angle = Chassis_Angle - Reference_Angle;

    // 云台坐标系的目标速度转为底盘坐标系的目标速度
    chassis_velocity_x = (float)(gimbal_velocity_x * cos(derta_angle) - gimbal_velocity_y * sin(derta_angle));
    chassis_velocity_y = (float)(gimbal_velocity_x * sin(derta_angle) + gimbal_velocity_y * cos(derta_angle));

    // 设定底盘控制类型
    Chassis.Set_Chassis_Control_Type(tmp_chassis_control_type);

    // 底盘控制方案
    if (Chassis.Get_Chassis_Control_Type() == Chassis_Control_Type_SPIN)
    {
        target_omega = Chassis.Get_Spin_Omega();
    }
    else if (Chassis.Get_Chassis_Control_Type() == Chassis_Control_Type_FLLOW)
    {
        Chassis.PID_Chassis_Fllow.Set_Target(Reference_Angle);
        Chassis.PID_Chassis_Fllow.Set_Now(Chassis.Motor_Yaw.Get_Now_Angle());
        Chassis.PID_Chassis_Fllow.TIM_Adjust_PeriodElapsedCallback();
        target_omega = Chassis.PID_Chassis_Fllow.Get_Out();
    }
    else if (Chassis.Get_Chassis_Control_Type() == Chassis_Control_Type_DISABLE)
    {
        target_omega = 0;
    }
    else
    {
        target_omega = 0;
    }

    // 设定底盘目标速度
    Chassis.Set_Target_Velocity_X(chassis_velocity_x);
    Chassis.Set_Target_Velocity_Y(chassis_velocity_y);
    Chassis.Set_Target_Omega(-target_omega);
}
#endif

/**
 * @brief can回调函数处理底盘发来的数据
 *
 */
 #ifdef GIMBAL

can_rx1_t can_rx1;
can_rx2_t can_rx2;
can_rx3_t can_rx3;
struct Gimbal_RX_Chassis{
    Enum_Referee_Data_Robots_ID ID;
    Enum_Referee_Game_Status_Stage Game_stage; 
    uint16_t Shooter_Heat_Limit;
    uint16_t Shooter_Cooling_Value;
    uint16_t HP;
};
Gimbal_RX_Chassis Referee_Rx_Data;
Gimbal_RX_Chassis Pre_Referee_Rx_Data;

void Class_Chariot::CAN_Gimbal_Rx_Chassis_Callback()
{
    Chassis_Flag++;
    switch (CAN2_Manage_Object.Rx_Buffer.Header.StdId)
    {
    case (0x88):
    {
        memcpy(&Pre_Referee_Rx_Data,&Referee_Rx_Data,sizeof(Gimbal_RX_Chassis));
        memcpy(&Referee_Rx_Data, &CAN2_Manage_Object.Rx_Buffer.Data, sizeof(Gimbal_RX_Chassis));
    }
    break;
    }
}

float gimbal_velocity_x = 0, gimbal_velocity_y = 0;
float tmp_chassis_velocity_x = 0, tmp_chassis_velocity_y = 0,tmp_chassis_velocity_w = 0;
int16_t chassis_velocity_x = 0, chassis_velocity_y = 0, chassis_velocity_w = 0;
/**
 * @brief can云台向底盘发送数据
 *
 */
void Class_Chariot::CAN_Gimbal_Tx_Chassis_Callback()
{
    // 云台坐标系速度目标值 float

    float relative_angle = 0;
    
    Enum_Chassis_Control_Type chassis_control_type;

    float gimbal_angle = 0, chassis_angle = 0;
    //上位机优先
    if(MiniPC.Get_MiniPC_Status() == MiniPC_Status_ENABLE && DR16.Get_Left_Switch() == DR16_Switch_Status_DOWN){    
        if(MiniPC.Get_Chassis_Target_Velocity_X()!=0 || MiniPC.Get_Chassis_Target_Velocity_Y()!=0){
            Chassis.Set_Target_Velocity_X(-MiniPC.Get_Chassis_Target_Velocity_X());//mention the x-y-z axis transformation
            Chassis.Set_Target_Velocity_Y(-MiniPC.Get_Chassis_Target_Velocity_Y());
        }
    }
    // 设定速度
    gimbal_velocity_x = Chassis.Get_Target_Velocity_X();
    gimbal_velocity_y = Chassis.Get_Target_Velocity_Y();

    gimbal_angle = Gimbal.Get_Gimbal_Head_Angle();
    chassis_angle = Gimbal.Motor_Yaw.Get_Now_Angle();
    relative_angle = gimbal_angle - chassis_angle;
    MiniPC.Set_Gimbal_Now_Relative_Angle(RAD_TO_ANGEL(relative_angle));
    //上板->下板通信
    chassis_control_type = Chassis.Get_Chassis_Control_Type();
    volatile int Chassis_control_type = Chassis.Get_Chassis_Control_Type();  
    switch(Chassis_control_type){
        case(Chassis_Control_Type_FLLOW):{
            if(Gimbal.Motor_Yaw.Get_DJI_Motor_Status() == DJI_Motor_Status_DISABLE){
                chassis_control_type = Chassis_Control_Type_DISABLE;           
            }
            else{
                Chassis.Chassis_Follow_PID_Angle.Set_Target(0);
                Chassis.Chassis_Follow_PID_Angle.Set_Now(RAD_TO_DEG(relative_angle));
                Chassis.Chassis_Follow_PID_Angle.TIM_Adjust_PeriodElapsedCallback();
                tmp_chassis_velocity_w = Chassis.Chassis_Follow_PID_Angle.Get_Out();
                tmp_chassis_velocity_x = gimbal_velocity_x * cos(relative_angle) + gimbal_velocity_y * sin(relative_angle);
                tmp_chassis_velocity_y = -gimbal_velocity_x * sin(relative_angle) + gimbal_velocity_y * cos(relative_angle);
            }
            if(MiniPC.Get_MiniPC_Status() != MiniPC_Status_DISABLE){
                if( MiniPC.Get_Chassis_Control_Mode() == MiniPC_Chassis_Control_Mode_NORMAL || 
                    MiniPC.Get_Chassis_Control_Mode() == MiniPC_Chassis_Control_Mode_NORMAL_SPIN)
                    tmp_chassis_velocity_w = 0;//不随动
                    if(Pre_Referee_Rx_Data.HP > Referee_Rx_Data.HP){
                        tmp_chassis_velocity_w = 3;
                        tmp_chassis_velocity_x = gimbal_velocity_x * cos(relative_angle) + gimbal_velocity_y * sin(relative_angle);
                        tmp_chassis_velocity_y = -gimbal_velocity_x * sin(relative_angle) + gimbal_velocity_y * cos(relative_angle);
                    }
            }
            else if (MiniPC.Get_MiniPC_Status() == MiniPC_Status_DISABLE 
                  && DR16.Get_Left_Switch()     == DR16_Switch_Status_DOWN){
                    tmp_chassis_velocity_w = 0;//不随动+受击陀螺
                    if(Pre_Referee_Rx_Data.HP > Referee_Rx_Data.HP){
                        tmp_chassis_velocity_w = 3;
                        tmp_chassis_velocity_x = gimbal_velocity_x * cos(relative_angle) + gimbal_velocity_y * sin(relative_angle);
                        tmp_chassis_velocity_y = -gimbal_velocity_x * sin(relative_angle) + gimbal_velocity_y * cos(relative_angle);
                    }
            }
            break;
        }
        case(Chassis_Control_Type_SPIN):{
            tmp_chassis_velocity_w = 3;
            tmp_chassis_velocity_x = gimbal_velocity_x * cos(relative_angle) + gimbal_velocity_y * sin(relative_angle);
            tmp_chassis_velocity_y = -gimbal_velocity_x * sin(relative_angle) + gimbal_velocity_y * cos(relative_angle);
            break;
        }
        
    }
    
    
    //Type conversion and clipping
    chassis_velocity_x = Math_Float_To_Int(tmp_chassis_velocity_x, -1 * Chassis.Get_Velocity_X_Max(), Chassis.Get_Velocity_X_Max(), -450, 450);
    chassis_velocity_y = Math_Float_To_Int(tmp_chassis_velocity_y, -1 * Chassis.Get_Velocity_Y_Max(), Chassis.Get_Velocity_Y_Max(), -450, 450);
    chassis_velocity_w = Math_Float_To_Int(tmp_chassis_velocity_w, -1 * 4.0f, 4.0f, -200, 200);
    //失能保护
    if(DR16.Get_DR16_Status()==DR16_Status_DISABLE){
        chassis_velocity_x =0;
        chassis_velocity_y =0;
        chassis_velocity_w = 0;
    }
    //Can send data
    memcpy(CAN2_0x150_Tx_Data, &chassis_velocity_x, sizeof(int16_t));
    memcpy(CAN2_0x150_Tx_Data + 2, &chassis_velocity_y, sizeof(int16_t));
    memcpy(CAN2_0x150_Tx_Data + 4, &chassis_velocity_w, sizeof(int16_t));
    memcpy(CAN2_0x150_Tx_Data + 6, &chassis_control_type, sizeof(int8_t));
}

 #endif
/**
 * @brief 底盘控制逻辑
 *
 */
#ifdef GIMBAL
void Class_Chariot::Control_Chassis()
{
    // 遥控器摇杆值
    float dr16_l_x, dr16_l_y;
    // 云台坐标系速度目标值 float
    float gimbal_velocity_x = 0, gimbal_velocity_y = 0;

    // 遥控器操作逻辑

    // 排除遥控器死区
    dr16_l_x = (Math_Abs(DR16.Get_Left_X()) > DR16_Dead_Zone) ? DR16.Get_Left_X() : 0;
    dr16_l_y = (Math_Abs(DR16.Get_Left_Y()) > DR16_Dead_Zone) ? DR16.Get_Left_Y() : 0;

    // 设定矩形到圆形映射进行控制
    gimbal_velocity_x = dr16_l_x * sqrt(1.0f - dr16_l_y * dr16_l_y / 2.0f) * Chassis.Get_Velocity_X_Max();
    gimbal_velocity_y = dr16_l_y * sqrt(1.0f - dr16_l_x * dr16_l_x / 2.0f) * Chassis.Get_Velocity_Y_Max();

    // 键盘遥控器操作逻辑
    volatile int DR16_Left_Switch_Status = DR16.Get_Left_Switch();
    switch(DR16_Left_Switch_Status){
        case (DR16_Switch_Status_UP):   // 左上 小陀螺模式
        {

            Chassis.Set_Chassis_Control_Type(Chassis_Control_Type_SPIN);
            Chassis.Set_Target_Velocity_Y(gimbal_velocity_y);
            Chassis.Set_Target_Velocity_X(gimbal_velocity_x);
            break;
        }
        case(DR16_Switch_Status_MIDDLE): // 左中 随动模式
        {
            Chassis.Set_Chassis_Control_Type(Chassis_Control_Type_FLLOW);
            Chassis.Set_Target_Velocity_Y(gimbal_velocity_y);
            Chassis.Set_Target_Velocity_X(gimbal_velocity_x);
            break;
        }
        case(DR16_Switch_Status_DOWN):  // 左下 上位机
        {
            
            if (MiniPC.Get_MiniPC_Status() == MiniPC_Status_DISABLE)
            {
                Chassis.Set_Chassis_Control_Type(Chassis_Control_Type_FLLOW);
                Chassis.Set_Target_Velocity_X(0);
                Chassis.Set_Target_Velocity_Y(0);
                break;
            }

            volatile int MiniPC_Chassis_control_type = MiniPC.Get_Chassis_Control_Mode();
            switch(MiniPC_Chassis_control_type)
            {
                case(MiniPC_Chassis_Control_Mode_NORMAL):{
                    Chassis.Set_Chassis_Control_Type(Chassis_Control_Type_FLLOW);
                }
                break;
                case(MiniPC_Chassis_Control_Mode_FOLLOW_SPIN):{
                    Chassis.Set_Chassis_Control_Type(Chassis_Control_Type_FLLOW);
                }
                break;
                case(MiniPC_Chassis_Control_Mode_FOLLOW):{
                    Chassis.Set_Chassis_Control_Type(Chassis_Control_Type_FLLOW);
                }
                break;
                case(MiniPC_Chassis_Control_Mode_SPIN):{
                    Chassis.Set_Chassis_Control_Type(Chassis_Control_Type_SPIN);
                }
                break;
                case(MiniPC_Chassis_Control_Mode_NORMAL_SPIN):{
                    Chassis.Set_Chassis_Control_Type(Chassis_Control_Type_FLLOW);
                }
                break;
            }
        }
    }
}
#endif
/**
 * @brief 云台控制逻辑
 *
 */
#ifdef GIMBAL
float tmp_test_gimbal_yaw;
void Class_Chariot::Control_Gimbal()
{
    // 角度目标值
    float tmp_gimbal_yaw, tmp_gimbal_pitch;
    // 遥控器摇杆值
    float dr16_y, dr16_r_y;

    // 排除遥控器死区
    dr16_y = (Math_Abs(DR16.Get_Right_X()) > DR16_Dead_Zone) ? DR16.Get_Right_X() : 0;
    dr16_r_y = (Math_Abs(DR16.Get_Right_Y()) > DR16_Dead_Zone) ? DR16.Get_Right_Y() : 0;

    tmp_gimbal_yaw = Gimbal.Motor_Yaw.Get_Target_Angle();
    tmp_gimbal_pitch = Gimbal.Motor_Pitch.Get_Target_Angle();

    // 遥控器操作逻辑
    tmp_gimbal_yaw -= dr16_y * DR16_Yaw_Angle_Resolution;
    tmp_gimbal_pitch += dr16_r_y * DR16_Pitch_Angle_Resolution;

    if (DR16.Get_Left_Switch() == DR16_Switch_Status_DOWN) // 左下 上位机
    {
        Gimbal.Set_Gimbal_Control_Type(Gimbal_Control_Type_MINIPC);
        
    }
    else // 其余位置都是遥控器控制
    {
        // 中间遥控模式
        Gimbal.Set_Gimbal_Control_Type(Gimbal_Control_Type_NORMAL);

        // 设定角度
        Gimbal.Set_Target_Yaw_Angle(tmp_gimbal_yaw);
        Gimbal.Set_Target_Pitch_Angle(tmp_gimbal_pitch);
    }

    
}
#endif
/**
 * @brief 发射机构控制逻辑
 *
 */
#ifdef GIMBAL
extern uint8_t Auto_aim_flag;
void Class_Chariot::Control_Booster()
{
    static uint8_t booster_sign = 0;

    volatile int DR16_Left_Switch_Status = DR16.Get_Left_Switch();
    switch (DR16_Left_Switch_Status)
    {
        case(DR16_Switch_Status_MIDDLE):
        {
            Booster.Set_Booster_Control_Type(Booster_Control_Type_DISABLE);            
        }
        break;
        case(DR16_Switch_Status_DOWN):
        {
            // if (DR16.Get_Right_Switch() != DR16_Switch_Status_DOWN) // 比赛未开始 不拨弹
            // {
            //     if (can_rx1.game_process != 4)
            //     {
            //         Booster.Set_Booster_Control_Type(Booster_Control_Type_CEASEFIRE);
            //         return;
            //     }
            // }
            if(Auto_aim_flag == 0){
                Booster.Set_Booster_Control_Type(Booster_Control_Type_MINIPC);
            }
            else{
                Booster.Set_Booster_Control_Type(Booster_Control_Type_CEASEFIRE);
            }
            break;
        }
    }

    if(DR16.Get_Right_Switch() == DR16_Switch_Status_DOWN){  //连发
        Booster.Set_Booster_Control_Type(Booster_Control_Type_REPEATED);
    }
    if (DR16.Get_Right_Switch() == DR16_Switch_Status_UP)
    {
        Booster.Set_Booster_Control_Type(Booster_Control_Type_CEASEFIRE);
        if (DR16.Get_Yaw() >= -0.2 && DR16.Get_Yaw() <= 0.2)
        {
            booster_sign = 0;
        }
        else if (DR16.Get_Yaw() >= 0.8 && booster_sign == 0) // 单发
        {
            Booster.Set_Booster_Control_Type(Booster_Control_Type_SINGLE);
            booster_sign = 1;
        }
        else if (DR16.Get_Yaw() <= -0.8 && booster_sign == 0) // 五连发
        {
            Booster.Set_Booster_Control_Type(Booster_Control_Type_MULTI);
            booster_sign = 1;
        }
    }

}

#endif

/**
 * @brief 计算回调函数
 *
 */
void Class_Chariot::TIM_Calculate_PeriodElapsedCallback()
{
#ifdef CHASSIS

    // 底盘的控制策略
    //    Control_Chassis();
    // 各个模块的分别解算
    Chassis.TIM_Calculate_PeriodElapsedCallback();

    Supercap.Set_Now_Power(Chassis.Referee->Get_Chassis_Power());
    Supercap.Set_Limit_Power(Chassis.Referee->Get_Chassis_Power_Max());
    Supercap.TIM_UART_PeriodElapsedCallback();

#elif defined(GIMBAL)

    // 各个模块的分别解算

    Gimbal.TIM_Calculate_PeriodElapsedCallback();

    Booster.TIM_Calculate_PeriodElapsedCallback();

    MiniPC.Set_Gimbal_Now_Pitch_Angle(Gimbal.Motor_Pitch.Get_True_Angle_Pitch());
    MiniPC.Set_Gimbal_Now_Yaw_Angle(Gimbal.Motor_Yaw.Get_True_Angle_Yaw());
   // MiniPC.Set_Chassis_Now_Velocity_X(Chassis.Get_Now_Velocity_X());

        // 传输数据给上位机
        MiniPC.TIM_Write_PeriodElapsedCallback();

    this->CAN_Gimbal_Tx_Chassis_Callback();

#endif
}

/**
 * @brief 控制回调函数
 *
 */
#ifdef GIMBAL
void Class_Chariot::TIM_Control_Callback()
{
    // 底盘，云台，发射机构控制逻辑
    Control_Chassis();
    Control_Gimbal();
    Control_Booster();
}
#endif
/**
 * @brief 在线判断回调函数
 *
 */
void Class_Chariot::TIM1msMod50_Alive_PeriodElapsedCallback()
{
    static int mod50 = 0;
    mod50++;
    if (mod50 == 50)
    {
#ifdef CHASSIS

        Referee.TIM1msMod50_Alive_PeriodElapsedCallback();

#ifdef POWER_LIMIT
        Supercap.TIM_Alive_PeriodElapsedCallback();
#endif

        for (auto &wheel : Chassis.Motor_Wheel)
        {
            wheel.TIM_Alive_PeriodElapsedCallback();
        }

#elif defined(GIMBAL)

        DR16.TIM1msMod50_Alive_PeriodElapsedCallback();

        // 记得解开注释
        if (DR16.Get_DR16_Status() == DR16_Status_DISABLE)
        {
            Gimbal.Set_Gimbal_Control_Type(Gimbal_Control_Type_DISABLE);
            Booster.Set_Booster_Control_Type(Booster_Control_Type_DISABLE);
            Chassis.Set_Chassis_Control_Type(Chassis_Control_Type_DISABLE);
        }

        Gimbal.Motor_Pitch.TIM_Alive_PeriodElapsedCallback();
        Gimbal.Motor_Yaw.TIM_Alive_PeriodElapsedCallback();
        Gimbal.Motor_Pitch_LK6010.TIM_Alive_PeriodElapsedCallback();
        Gimbal.Boardc_BMI.TIM1msMod50_Alive_PeriodElapsedCallback();

        Booster.Motor_Driver.TIM_Alive_PeriodElapsedCallback();
        Booster.Motor_Friction_Left.TIM_Alive_PeriodElapsedCallback();
        Booster.Motor_Friction_Right.TIM_Alive_PeriodElapsedCallback();

        MiniPC.TIM1msMod50_Alive_PeriodElapsedCallback();
        Referee.TIM1msMod50_Alive_PeriodElapsedCallback();
        TIM1msMod50_Chassis_Communicate_Alive_PeriodElapsedCallback(); // 和底盘通讯存活状态

#endif
        mod50 = 0;
    }
}

void Class_Chariot::TIM1msMod50_Chassis_Communicate_Alive_PeriodElapsedCallback()
{
    // 判断该时间段内是否接收过电机数据
    if (Chassis_Flag == Pre_Chassis_Flag)
    {
        // 电机断开连接
        Chassis_Status = Chassis_Status_DISABLE;
    }
    else
    {
        // 电机保持连接
        Chassis_Status = Chassis_Status_ENABLE;
    }
    Pre_Chassis_Flag = Chassis_Flag;
}

/************************ COPYRIGHT(C) USTC-ROBOWALKER **************************/
