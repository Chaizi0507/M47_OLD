/**
 * @file alg_pid.h
 * @author yssickjgd (1345578933@qq.com)
 * @brief PID算法
 * @version 0.1
 * @date 2023-08-29 0.1 23赛季定稿
 *
 * @copyright USTC-RoboWalker (c) 2022
 *
 */

#ifndef ALG_PID_H
#define ALG_PID_H

/* Includes ------------------------------------------------------------------*/

#include "drv_math.h"

/* Exported macros -----------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/

/**
 * @brief 微分先行
 *
 */
enum Enum_PID_D_First
{
    PID_D_First_DISABLE = 0,
    PID_D_First_ENABLE,
};

/**
 * @brief Reusable, PID算法
 *
 */
class Class_PID
{
public:
    void Init(double __K_P, double __K_I, double __K_D, double __K_F = 0.0f, double __I_Out_Max = 0.0f, double __Out_Max = 0.0f, double __I_Variable_Speed_A = 0.0f, double __I_Variable_Speed_B = 0.0f, double __I_Separate_Threshold = 0.0f, double __D_T = 0.001f, double __Dead_Zone = 0.0f, Enum_PID_D_First __D_First = PID_D_First_DISABLE);

    inline double Get_Integral_Error();
    inline double Get_Out();

    inline void Set_K_P(double __K_P);
    inline void Set_K_I(double __K_I);
    inline void Set_K_D(double __K_D);
    inline void Set_K_F(double __K_F);
    inline void Set_I_Out_Max(double __I_Out_Max);
    inline void Set_Out_Max(double __Out_Max);
    inline void Set_I_Variable_Speed_A(double __Variable_Speed_I_A);
    inline void Set_I_Variable_Speed_B(double __Variable_Speed_I_B);
    inline void Set_I_Separate_Threshold(double __I_Separate_Threshold);
    inline void Set_Target(double __Target);
    inline void Set_Now(double __Now);
    inline void Set_Integral_Error(double __Integral_Error);

    void TIM_Adjust_PeriodElapsedCallback();

protected:
    //初始化相关常量

    // PID计时器周期, s
    double D_T;
    //死区, Error在其绝对值内不输出
    double Dead_Zone;
    //微分先行
    Enum_PID_D_First D_First;

    //常量

    //内部变量

    //之前的当前值
    double Pre_Now = 0.0f;
    //之前的目标值
    double Pre_Target = 0.0f;
    //之前的输出值
    double Pre_Out = 0.0f;
    //前向误差
    double Pre_Error = 0.0f;

    //读变量
    double Now_Error = 0.0f;
        // 输出值
        double Out = 0.0f;
    double I_out = 0.0f;
    // 写变量

    // PID的P
    double K_P = 0.0f;
    // PID的I
    double K_I = 0.0f;
    // PID的D
    double K_D = 0.0f;
    //前馈
    double K_F = 0.0f;

    //积分限幅, 0为不限制
    double I_Out_Max = 0;
    //输出限幅, 0为不限制
    double Out_Max = 0;

    //变速积分定速内段阈值, 0为不限制
    double I_Variable_Speed_A = 0.0f;
    //变速积分变速区间, 0为不限制
    double I_Variable_Speed_B = 0.0f;
    //积分分离阈值，需为正数, 0为不限制
    double I_Separate_Threshold = 0.0f;

    //目标值
    double Target = 0.0f;
    //当前值
    double Now = 0.0f;

    //读写变量

    //积分值
    double Integral_Error = 0.0f;

    //内部函数
};

/* Exported variables --------------------------------------------------------*/

/* Exported function declarations --------------------------------------------*/

/**
 * @brief 获取输出值
 *
 * @return double 输出值
 */
double Class_PID::Get_Integral_Error()
{
    return (Integral_Error);
}

/**
 * @brief 获取输出值
 *
 * @return double 输出值
 */
double Class_PID::Get_Out()
{
    return (Out);
}

/**
 * @brief 设定PID的P
 *
 * @param __K_P PID的P
 */
void Class_PID::Set_K_P(double __K_P)
{
    K_P = __K_P;
}

/**
 * @brief 设定PID的I
 *
 * @param __K_I PID的I
 */
void Class_PID::Set_K_I(double __K_I)
{
    K_I = __K_I;
}

/**
 * @brief 设定PID的D
 *
 * @param __K_D PID的D
 */
void Class_PID::Set_K_D(double __K_D)
{
    K_D = __K_D;
}

/**
 * @brief 设定前馈
 *
 * @param __K_D 前馈
 */
void Class_PID::Set_K_F(double __K_F)
{
    K_F = __K_F;
}

/**
 * @brief 设定积分限幅, 0为不限制
 *
 * @param __I_Out_Max 积分限幅, 0为不限制
 */
void Class_PID::Set_I_Out_Max(double __I_Out_Max)
{
    I_Out_Max = __I_Out_Max;
}

/**
 * @brief 设定输出限幅, 0为不限制
 *
 * @param __Out_Max 输出限幅, 0为不限制
 */
void Class_PID::Set_Out_Max(double __Out_Max)
{
    Out_Max = __Out_Max;
}

/**
 * @brief 设定定速内段阈值, 0为不限制
 *
 * @param __I_Variable_Speed_A 定速内段阈值, 0为不限制
 */
void Class_PID::Set_I_Variable_Speed_A(double __I_Variable_Speed_A)
{
    I_Variable_Speed_A = __I_Variable_Speed_A;
}

/**
 * @brief 设定变速区间, 0为不限制
 *
 * @param __I_Variable_Speed_B 变速区间, 0为不限制
 */
void Class_PID::Set_I_Variable_Speed_B(double __I_Variable_Speed_B)
{
    I_Variable_Speed_B = __I_Variable_Speed_B;
}

/**
 * @brief 设定积分分离阈值，需为正数, 0为不限制
 *
 * @param __I_Separate_Threshold 积分分离阈值，需为正数, 0为不限制
 */
void Class_PID::Set_I_Separate_Threshold(double __I_Separate_Threshold)
{
    I_Separate_Threshold = __I_Separate_Threshold;
}

/**
 * @brief 设定目标值
 *
 * @param __Target 目标值
 */
void Class_PID::Set_Target(double __Target)
{
    Target = __Target;
}

/**
 * @brief 设定当前值
 *
 * @param __Now 当前值
 */
void Class_PID::Set_Now(double __Now)
{
    Now = __Now;
}

/**
 * @brief 设定积分, 一般用于积分清零
 *
 * @param __Set_Integral_Error 积分值
 */
void Class_PID::Set_Integral_Error(double __Integral_Error)
{
    Integral_Error = __Integral_Error;
}

#endif

/************************ COPYRIGHT(C) USTC-ROBOWALKER **************************/
