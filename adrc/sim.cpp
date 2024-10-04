#include <iostream>

class Motor {
public:
    Motor(double k_t=0.1, double damping=0.01, double inertia=0.05)
        : k_t(k_t), damping(damping), inertia(inertia), speed(0) {}

    double applyCurrent(double current, double dt) {
        // 计算加速度：转矩 = k_t * 电流，减去阻尼力
        double torque = k_t * current;
        double damping_force = damping * speed;
        double acceleration = (torque - damping_force) / inertia;

        // 更新速度
        speed += acceleration * dt;
        return speed;
    }

private:
    double k_t;       // 转矩常数
    double damping;   // 阻尼系数
    double inertia;   // 惯性系数
    double speed;     // 当前速度
};

int main() {
    Motor motor;

    double current_input = 5.0; // 输入电流（安培）
    double dt = 0.1;            // 时间步长（秒）
    int steps = 100;            // 模拟步数

    for (int i = 0; i < steps; ++i) {
        double speed_output = motor.applyCurrent(current_input, dt);
        std::cout << "时间: " << i * dt 
                  << " 秒，输入电流: " << current_input 
                  << " A，输出速度: " << speed_output 
                  << " m/s" << std::endl;
    }

    return 0;
}
