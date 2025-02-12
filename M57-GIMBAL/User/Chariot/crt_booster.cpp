/**
 * @file crt_booster.cpp
 * @author yssickjgd (yssickjgd@mail.ustc.edu.cn)
 * @brief 发射机构电控
 * @version 0.1
 * @date 2023-08-29 0.1 23赛季定稿
 *
 * @copyright USTC-RoboWalker (c) 2022
 *
 */

/* Includes ------------------------------------------------------------------*/

#include "crt_booster.h"
#include "dvc_dwt.h"
/* Private macros ------------------------------------------------------------*/

/* Private types -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Private function declarations ---------------------------------------------*/

/* Function prototypes -------------------------------------------------------*/

/**
 * @brief 定时器处理函数
 * 这是一个模板, 使用时请根据不同处理情况在不同文件内重新定义
 *
 */
void Class_FSM_Heat_Detect::Reload_TIM_Status_PeriodElapsedCallback()
{
    Status[Now_Status_Serial].Time++;

    // 自己接着编写状态转移函数
    switch (Now_Status_Serial)
    {
    case (0):
    {
        // 正常状态

        if (abs(Booster->Motor_Friction_Right.Get_Now_Torque()) >= Booster->Friction_Torque_Threshold)
        {
            // 大扭矩->检测状态
            Set_Status(1);
        }
        else if (Booster->Booster_Control_Type == Booster_Control_Type_DISABLE)
        {
            // 停机->停机状态
            Set_Status(3);
        }
    }
    break;
    case (1):
    {
        // 发射嫌疑状态

        if (Status[Now_Status_Serial].Time >= 15)
        {
            // 长时间大扭矩->确认是发射了
            Set_Status(2);
        }
    }
    break;
    case (2):
    {
        // 发射完成状态->加上热量进入下一轮检测

        Heat += 10.0f;
        Set_Status(0);
    }
    break;
    case (3):
    {
        // 停机状态

        if (abs(Booster->Motor_Friction_Right.Get_Now_Omega()) >= Booster->Friction_Omega_Threshold)
        {
            // 开机了->正常状态
            Set_Status(0);
        }
    }
    break;
    }

    // 热量冷却到0
    if (Heat > 0)
    {
        // Heat -= Booster->Referee->Get_Booster_17mm_1_Heat_CD() / 1000.0f;
    }
    else
    {
        Heat = 0;
    }
}
int jam_flag = 0;
/**
 * @brief 卡弹策略有限自动机
 *
 */

void Class_FSM_Antijamming::Reload_TIM_Status_PeriodElapsedCallback()
{
    Status[Now_Status_Serial].Time++;

    // 自己接着编写状态转移函数
    switch (Now_Status_Serial)
    {
    case (0):
    {
        // 正常状态
        Booster->Output();

        if (abs(Booster->Motor_Driver.Get_Now_Torque()) >= Booster->Driver_Torque_Threshold)
        {
            // 大扭矩+低速->卡弹嫌疑状态
            Set_Status(1);
        }
    }
    break;
    case (1):
    {
        // 卡弹嫌疑状态
        Booster->Output();

        if (Status[Now_Status_Serial].Time >= 200)
        {
            // 长时间大扭矩->卡弹反应状态
            Set_Status(2);
        }
        else if (abs(Booster->Motor_Driver.Get_Now_Torque()) <= Booster->Driver_Torque_Threshold )
        {
            // 短时间大扭矩+低速->正常状态
            Set_Status(0);
        }
    }
    break;
    case (2):
    {
        // 卡弹反应状态->准备卡弹处理
        Booster->Motor_Driver.Set_DJI_Motor_Control_Method(DJI_Motor_Control_Method_ANGLE);
		//Booster->Driver_Angle = Booster->Motor_Driver.Get_Now_Angle() - PI / 6.0f;  新哨兵
        Booster->Driver_Angle = Booster->Motor_Driver.Get_Now_Angle() - PI / 12.0f*3.86;
        Booster->Motor_Driver.Set_Target_Angle(Booster->Driver_Angle);
        jam_flag = 1;
        Set_Status(3);
    }
    break;
    case (3):
    {
        // 卡弹处理状态
        if (Status[Now_Status_Serial].Time >= 400)
        {
            // 长时间回拨->正常状态
            Set_Status(0);
        }
        jam_flag = 0;
    }
    break;
    }
}

/**
 * @brief 发射机构初始化
 *
 */
void Class_Booster::Init()
{

    
    // 正常状态, 发射嫌疑状态, 发射完成状态, 停机状态
    FSM_Heat_Detect.Booster = this;
    FSM_Heat_Detect.Init(3, 3);

    // 正常状态, 卡弹嫌疑状态, 卡弹反应状态, 卡弹处理状态
    FSM_Antijamming.Booster = this;
    FSM_Antijamming.Init(4, 0);

    // 拨弹盘电机
    Motor_Driver.PID_Angle.Init(75.0f, 0.0f, 1.0f, 0.0f, Default_Driver_Omega, Default_Driver_Omega);
    Motor_Driver.PID_Omega.Init(3500.0f, 800.0f, 0.0f, 0.0f, 10000.f, 16000.f);
    Motor_Driver.Init(&hcan2, DJI_Motor_ID_0x203, DJI_Motor_Control_Method_OMEGA);//normal
    //Motor_Driver.Init(&hcan2, DJI_Motor_ID_0x201, DJI_Motor_Control_Method_OMEGA,36.0f * 2.32f); 新哨兵
    //Motor_Driver.Init(&hcan1, DJI_Motor_ID_0x201, DJI_Motor_Control_Method_ANGLE);//test

    // 摩擦轮电机左
    Motor_Friction_Left.PID_Omega.Init(120.0f, 10.0f, 0.1f, 0.0f, 2000.0f, Motor_Friction_Left.Get_Output_Max());
    Motor_Friction_Left.Init(&hcan1, DJI_Motor_ID_0x201, DJI_Motor_Control_Method_OMEGA, 1.0f);

    // 摩擦轮电机右
    Motor_Friction_Right.PID_Omega.Init(120.0f, 10.0f, 0.1f, 0.0f, 2000.0f, Motor_Friction_Right.Get_Output_Max());
    Motor_Friction_Right.Init(&hcan1, DJI_Motor_ID_0x202, DJI_Motor_Control_Method_OMEGA, 1.0f);
}

/**
 * @brief 输出到电机
 *
 */

void Class_Booster::Output()
{
    // Now_Angle = Motor_Driver.Get_Now_Angle();
    // Motor_Driver.Set_DJI_Motor_Control_Method(DJI_Motor_Control_Method_ANGLE);
    // Driver_Angle = Now_Angle + 2.0f * PI / 12.0f*2.32 * 5.0f; // 五连发
    // Motor_Driver.Set_Target_Angle(Driver_Angle);
    Now_Angle = Motor_Driver.Get_Now_Angle();
    switch (Booster_Control_Type)
    {
    case (Booster_Control_Type_DISABLE):
    {
        // 发射机构失能
        Motor_Driver.Set_DJI_Motor_Control_Method(DJI_Motor_Control_Method_ANGLE);
        Motor_Friction_Left.Set_DJI_Motor_Control_Method(DJI_Motor_Control_Method_OMEGA);
        Motor_Friction_Right.Set_DJI_Motor_Control_Method(DJI_Motor_Control_Method_OMEGA);

        Motor_Driver.PID_Angle.Set_Integral_Error(0.0f);
        Motor_Driver.PID_Omega.Set_Integral_Error(0.0f);
        Motor_Friction_Left.PID_Angle.Set_Integral_Error(0.0f);
        Motor_Friction_Right.PID_Angle.Set_Integral_Error(0.0f);

        Motor_Driver.Set_Target_Angle(Driver_Angle);
        Motor_Friction_Left.Set_Target_Omega(0.0f);
        Motor_Friction_Right.Set_Target_Omega(0.0f);
    }
    break;
    case (Booster_Control_Type_CEASEFIRE):
    {
        // 停火
        if (Motor_Driver.Get_Control_Method() == DJI_Motor_Control_Method_ANGLE)
        {
            Motor_Driver.Set_Target_Angle(Driver_Angle);
        }
        else if (Motor_Driver.Get_Control_Method() == DJI_Motor_Control_Method_OMEGA)
        {
            Motor_Driver.Set_Target_Omega(0.0f);
        }
        Motor_Friction_Left.Set_Target_Omega(-Friction_Omega);
        Motor_Friction_Right.Set_Target_Omega(Friction_Omega);//检查布线 可能是反的
    }
    break;
    case (Booster_Control_Type_SINGLE):
    {
        // 单发模式
        Motor_Driver.Set_DJI_Motor_Control_Method(DJI_Motor_Control_Method_ANGLE);
        Motor_Friction_Left.Set_DJI_Motor_Control_Method(DJI_Motor_Control_Method_OMEGA);
        Motor_Friction_Right.Set_DJI_Motor_Control_Method(DJI_Motor_Control_Method_OMEGA);

        // 热量控制
        // if (Referee->Get_Booster_17mm_1_Heat() + 30 < Referee->Get_Booster_17mm_1_Heat_Max())
        // {
            Driver_Angle = Now_Angle + 2.0f * PI / 12.0f*2.32;
            Motor_Driver.Set_Target_Angle(Driver_Angle);
        //}

        Motor_Friction_Left.Set_Target_Omega(-Friction_Omega);
        Motor_Friction_Right.Set_Target_Omega(Friction_Omega);
    }
    break;
    case (Booster_Control_Type_MULTI):
    {
        // 五发模式
        Motor_Driver.Set_DJI_Motor_Control_Method(DJI_Motor_Control_Method_ANGLE);
        Motor_Friction_Left.Set_DJI_Motor_Control_Method(DJI_Motor_Control_Method_OMEGA);
        Motor_Friction_Right.Set_DJI_Motor_Control_Method(DJI_Motor_Control_Method_OMEGA);

        // 热量控制
        // if (Referee->Get_Booster_17mm_1_Heat() + 30< Referee->Get_Booster_17mm_1_Heat_Max())
        // {
            Driver_Angle = Now_Angle + 2.0f * PI / 12.0f*2.32 * 5.0f; // 五连发
            Motor_Driver.Set_Target_Angle(Driver_Angle);
        // }
        Motor_Friction_Left.Set_Target_Omega(-Friction_Omega);
        Motor_Friction_Right.Set_Target_Omega(Friction_Omega);
    }
    break;
    case (Booster_Control_Type_REPEATED):
    {
        // 连发模式
        Motor_Driver.Set_DJI_Motor_Control_Method(DJI_Motor_Control_Method_OMEGA);
        Motor_Friction_Left.Set_DJI_Motor_Control_Method(DJI_Motor_Control_Method_OMEGA);
        Motor_Friction_Right.Set_DJI_Motor_Control_Method(DJI_Motor_Control_Method_OMEGA);

        // 热量控制
        // if (Referee->Get_Booster_17mm_1_Heat() + 30 < Referee->Get_Booster_17mm_1_Heat_Max())
        // {
             Motor_Driver.Set_Target_Omega(Default_Driver_Omega);
        // }
        // else
        // {
        //     Motor_Driver.Set_Target_Omega(0);
        // }
        Driver_Angle = Motor_Driver.Get_Now_Angle();
        Motor_Friction_Left.Set_Target_Omega(-Friction_Omega);
        Motor_Friction_Right.Set_Target_Omega(Friction_Omega);
    }
    break;
    case (Booster_Control_Type_MINIPC):
    {
        // 上位机模式
        Motor_Driver.Set_DJI_Motor_Control_Method(DJI_Motor_Control_Method_OMEGA);
        Motor_Friction_Left.Set_DJI_Motor_Control_Method(DJI_Motor_Control_Method_OMEGA);
        Motor_Friction_Right.Set_DJI_Motor_Control_Method(DJI_Motor_Control_Method_OMEGA);

        // 判断打弹
        if(abs(MiniPC.Get_Gimbal_Error())<0.05)
            Motor_Driver.Set_Target_Omega(Default_Driver_Omega);
        else
            Motor_Driver.Set_Target_Omega(0);

        // 热量控制
        // if (Referee->Get_Booster_17mm_1_Heat() + 40 >= Referee->Get_Booster_17mm_1_Heat_Max())
        //     Motor_Driver.Set_Target_Omega(0);
            
        Driver_Angle = Motor_Driver.Get_Now_Angle();
        Motor_Friction_Left.Set_Target_Omega(-Friction_Omega);
        Motor_Friction_Right.Set_Target_Omega(Friction_Omega);
    }
    break;
    }
}

/**
 * @brief 定时器计算函数
 *
 */
void Class_Booster::TIM_Calculate_PeriodElapsedCallback()
{
    
    
    // 无需裁判系统的热量控制计算
    FSM_Heat_Detect.Reload_TIM_Status_PeriodElapsedCallback();
    //卡弹处理
    //FSM_Antijamming.Reload_TIM_Status_PeriodElapsedCallback();
    Output();

    Motor_Driver.TIM_PID_PeriodElapsedCallback();
    Motor_Friction_Left.TIM_PID_PeriodElapsedCallback();
    Motor_Friction_Right.TIM_PID_PeriodElapsedCallback();
}

/************************ COPYRIGHT(C) USTC-ROBOWALKER **************************/
