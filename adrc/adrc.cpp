#include "Adrc.h"
#include "math.h"

float Constrain_Float(float amt, float low, float high) {
	return ((amt) < (low) ? (low) : ((amt) > (high) ? (high) : (amt)));
}

int Sign_ADRC(float Input)
{
	int output = 0;
	if (Input > 1E-6f) output = 1;
	else if (Input < -1E-6f) output = -1;
	else output = 0;
	return output;
}

int Fsg_ADRC(float x, float d)
{
	int output = 0;
	output = (Sign_ADRC(x + d) - Sign_ADRC(x - d)) / 2.f;
	return output;
}

void ADRC_Init(Fhan_Data* fhan_Input, float r, float h, float z, float p, float d)
{
	fhan_Input->r = r;
	fhan_Input->h = h;
	fhan_Input->N0 = 3;
	fhan_Input->beta_01 = 1.f/h;
	fhan_Input->beta_02 = 1.f / powf(h, 1.5f) / 1.6f;
	fhan_Input->beta_03 = 1.f / powf(h, 2.2f) / 8.6f;
	fhan_Input->b0 = 0.001f;//扰动补偿，暂时不启用
	fhan_Input->beta_0 = 0;//控制律积分环节，不启用
	fhan_Input->beta_1 = p;
	fhan_Input->beta_2 = d;

	fhan_Input->alpha1 = 0.8f;
	fhan_Input->alpha2 = 1.5f;
	fhan_Input->zeta = z;
	fhan_Input->b = 0;//ESO扰动补偿，暂时不启用
	fhan_Input->alpha0 = -0.2f;//控制律积分环节非线性参数，不启用
	fhan_Input->e0 = 0;
}

//最速跟踪微分器
void Fhan_ADRC(Fhan_Data* fhan_Input, float expect_ADRC)//安排ADRC过度过程
{
	float d = 0, a0 = 0, y = 0, a1 = 0, a2 = 0, a = 0;
	float x1_delta = 0;//ADRC状态跟踪误差项
	x1_delta = fhan_Input->x1 - expect_ADRC;//用x1-v(k)替代x1得到离散更新公式
	fhan_Input->h0 = fhan_Input->N0 * fhan_Input->h;//用h0替代h，解决最速跟踪微分器速度超调问题
	d = fhan_Input->r * fhan_Input->h0 * fhan_Input->h0;//d=rh^2;
	a0 = fhan_Input->h0 * fhan_Input->x2;//a0=h*x2
	y = x1_delta + a0;//y=x1+a0
	a1 = sqrtf(d * (d + 8.f * fabsf(y)));//a1=sqrt(d*(d+8*fabsf(y))])
	a2 = a0 + Sign_ADRC(y) * (a1 - d) / 2;//a2=a0+sign(y)*(a1-d)/2;
	a = (a0 + y) * Fsg_ADRC(y, d) + a2 * (1 - Fsg_ADRC(y, d));
	fhan_Input->fh = -fhan_Input->r * (a / d) * Fsg_ADRC(a, d)
		- fhan_Input->r * Sign_ADRC(a) * (1 - Fsg_ADRC(a, d));//得到最速微分加速度跟踪量
	fhan_Input->x1 += fhan_Input->h * fhan_Input->x2;//跟新最速跟踪状态量x1
	fhan_Input->x2 += fhan_Input->h * fhan_Input->fh;//跟新最速跟踪状态量微分x2
}


//原点附近有连线性段的连续幂次函数
float Fal_ADRC(float e, float alpha, float zeta)
{
	//int s = 0;
	float fal_output = 0;
	//s = (Sign_ADRC(e + zeta) - Sign_ADRC(e - zeta)) / 2.f;
	if (fabsf(e) <= zeta)fal_output = e / powf(zeta, 1.f - alpha);
	else fal_output = Sign_ADRC(e) * powf(fabsf(e), alpha);
	//fal_output = e * s / (powf(zeta, 1 - alpha)) + powf(fabsf(e), alpha) * Sign_ADRC(e) * (1 - s);
	return fal_output;
}

/************扩张状态观测器********************/
//状态观测器参数beta01=1/h  beta02=1/(3*h^2)  beta03=2/(8^2*h^3) ...
void ESO_ADRC(Fhan_Data* fhan_Input)
{
	fhan_Input->e = fhan_Input->z1 - fhan_Input->y;//状态误差

	fhan_Input->fe = Fal_ADRC(fhan_Input->e, 0.5f, fhan_Input->h);//非线性函数，提取跟踪状态与当前状态误差
	fhan_Input->fe1 = Fal_ADRC(fhan_Input->e, 0.25f, fhan_Input->h);

	/*************扩展状态量更新**********/
	fhan_Input->z1 += fhan_Input->h * (fhan_Input->z2 - fhan_Input->beta_01 * fhan_Input->e);
	fhan_Input->z2 += fhan_Input->h * (fhan_Input->z3
		- fhan_Input->beta_02 * fhan_Input->fe
		+ fhan_Input->b * fhan_Input->u);
	//ESO估计状态加速度信号，进行扰动补偿，传统MEMS陀螺仪漂移较大，估计会产生漂移
	fhan_Input->z3 += fhan_Input->h * (-fhan_Input->beta_03 * fhan_Input->fe1);
}

void Nolinear_Conbination_ADRC(Fhan_Data* fhan_Input)
{
	float temp_e2 = 0;
	temp_e2 = Constrain_Float(fhan_Input->e2, -300, 300);

	fhan_Input->u0 = fhan_Input->beta_1 * Fal_ADRC(fhan_Input->e1, fhan_Input->alpha1, fhan_Input->zeta)
		//+ fhan_Input->beta_0 * Fal_ADRC(fhan_Input->e0, fhan_Input->alpha0, fhan_Input->zeta)
		+ fhan_Input->beta_2 * Fal_ADRC(temp_e2, fhan_Input->alpha2, fhan_Input->zeta);
}

void ADRC_Control(Fhan_Data* fhan_Input, float expect_ADRC, float feedback_ADRC)
{

	Fhan_ADRC(fhan_Input, expect_ADRC);

	fhan_Input->y = feedback_ADRC;

	ESO_ADRC(fhan_Input);

	fhan_Input->e0 += fhan_Input->e1 * fhan_Input->h;//状态积分项
	fhan_Input->e1 = fhan_Input->x1 - fhan_Input->z1;//状态偏差项
	fhan_Input->e2 = fhan_Input->x2 - fhan_Input->z2;//状态微分项，
	Nolinear_Conbination_ADRC(fhan_Input);
	fhan_Input->u = Constrain_Float(fhan_Input->u0, -500, 500);
}

