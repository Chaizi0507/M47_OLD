/**
 * @file alg_pid.cpp
 * @author yssickjgd 1345578933@qq.com
 * @brief PID算法
 * @version 0.1
 * @date 2023-08-29 0.1 23赛季定稿
 *
 * @copyright USTC-RoboWalker (c) 2022
 *
 */

/* Includes ------------------------------------------------------------------*/

#include "alg_pid.h"

/* Private macros ------------------------------------------------------------*/

/* Private types -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Private function declarations ---------------------------------------------*/

/* Function prototypes -------------------------------------------------------*/

/**
 * @brief PID初始化
 *
 * @param __K_P P值
 * @param __K_I I值
 * @param __K_D D值
 * @param __K_F 前馈
 * @param __I_Out_Max 积分限幅
 * @param __Out_Max 输出限幅
 * @param __D_T 时间片长度
 */
void Class_PID::Init(double __K_P, double __K_I, double __K_D, double __K_F, double __I_Out_Max, double __Out_Max, double __I_Variable_Speed_A, double __I_Variable_Speed_B, double __I_Separate_Threshold, double __D_T, double __Dead_Zone, Enum_PID_D_First __D_First)
{
    K_P = __K_P;
    K_I = __K_I;
    K_D = __K_D;
    K_F = __K_F;
    I_Out_Max = __I_Out_Max;
    Out_Max = __Out_Max;
    D_T = __D_T;
    Dead_Zone = __Dead_Zone;
    I_Variable_Speed_A = __I_Variable_Speed_A;
    I_Variable_Speed_B = __I_Variable_Speed_B;
    I_Separate_Threshold = __I_Separate_Threshold;
    D_First = __D_First;
}

/**
 * @brief PID调整值
 *
 * @return double 输出值
 */
void Class_PID::TIM_Adjust_PeriodElapsedCallback()
{
    // P输出
    double p_out = 0.0f;
    // I输出
    double i_out = 0.0f;
    // D输出
    double d_out = 0.0f;
    // F输出
    double f_out = 0.0f;
    // 误差
    double error;
    // 绝对值误差
    double abs_error;
    // 线性变速积分
    double speed_ratio;

    error = Target - Now;
    abs_error = Math_Abs(error);

    // 判断死区
    if (abs_error < Dead_Zone)
    {
        Target = Now;
        error = 0.0f;
        abs_error = 0.0f;
    }
    Now_Error=error;
        // 计算p项

        p_out = K_P * error;

    // 计算i项

    if (I_Variable_Speed_A == 0.0f && I_Variable_Speed_B == 0.0f)
    {
        // 非变速积分
        speed_ratio = 1.0f;
    }
    else
    {
        // 变速积分
        if (abs_error <= I_Variable_Speed_A)
        {
            speed_ratio = 1.0f;
        }
        else if (I_Variable_Speed_A < abs_error && abs_error < I_Variable_Speed_B)
        {
            speed_ratio = (I_Variable_Speed_B - abs_error) / (I_Variable_Speed_B - I_Variable_Speed_A);
        }
        if (abs_error >= I_Variable_Speed_B)
        {
            speed_ratio = 0.0f;
        }
    }
    // 积分限幅
    if (I_Out_Max != 0.0f)
    {
        Math_Constrain(&Integral_Error, -I_Out_Max / K_I, I_Out_Max / K_I);
    }
    if (I_Separate_Threshold == 0.0f)
    {
        // 没有积分分离
        Integral_Error += (float)(speed_ratio * D_T * error);
        if (isnan(Integral_Error))
        {
            Integral_Error = 0;
        }
        i_out = K_I * Integral_Error;
    }
    else
    {
        // 积分分离使能
        if (abs_error < I_Separate_Threshold)
        {
            Integral_Error += (float)(speed_ratio * D_T * error);
            if (isnan(Integral_Error))
            {
                Integral_Error = 0;
            }
            i_out = K_I * Integral_Error;
        }
        else
        {
            Integral_Error = 0.0f;
            i_out = 0.0f;
        }
    }
    I_out = i_out;
    // 计算d项

    if (D_First == PID_D_First_DISABLE)
    {
        // 没有微分先行
        d_out = K_D * (error - Pre_Error) / D_T;
    }
    else
    {
        // 微分先行使能
        d_out = K_D * (Out - Pre_Out) / D_T;
    }

    // 计算前馈

    f_out = (Target - Pre_Target) * K_F;

    // 计算总共的输出

    Out = p_out + i_out + d_out + f_out;
    // 输出限幅
    if (Out_Max != 0.0f)
    {
        Math_Constrain(&Out, -Out_Max, Out_Max);
    }

    // 善后工作
    Pre_Now = Now;
    Pre_Target = Target;
    Pre_Out = Out;
    Pre_Error = error;
}

/************************ COPYRIGHT(C) USTC-ROBOWALKER **************************/