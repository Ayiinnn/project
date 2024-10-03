#include "adrc.h"
#include <cmath>
#include <iostream>
using namespace std;
float ADRC_output(Fhan_Data* a)
{
    return a->u;
}
struct sim{

    Fhan_Data* testing;
    float target;
    const float td[5]={0.01,0.001,0.1,1,0.5};
    float result[100];
    float generator_a(float x){
        return 2*x+3*pow(x,2)+30;
    }

    void test(int times){
        int itr=0;
        ADRC_Init(testing,td[0],td[1],td[2],td[3],td[4]);
        while(itr<times)
        {
            float output=ADRC_output(testing);
            result[itr]=output;
            ADRC_Control(testing,target,generator_a(output));
            itr++;
        }
    }
};
int main(){
    sim tt1;
    tt1.target=100;
    tt1.test(20);
    for (int i=0;i<=19;i++)
    {
        cout<<tt1.result[i]<<" ";
    }
}