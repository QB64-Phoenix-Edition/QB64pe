/* Extra maths functions - we do what we must because we can */
inline double func_deg2rad(double value) { return (value * 0.01745329251994329576923690768489); }

inline double func_rad2deg(double value) { return (value * 57.29577951308232); }

inline double func_deg2grad(double value) { return (value * 1.111111111111111); }

inline double func_grad2deg(double value) { return (value * 0.9); }

inline double func_rad2grad(double value) { return (value * 63.66197723675816); }

inline double func_grad2rad(double value) { return (value * .01570796326794896); }

inline constexpr double func_pi(double multiplier, int32_t passed) {
    if (passed) {
        return 3.14159265358979323846264338327950288419716939937510582 * multiplier;
    }
    return (3.14159265358979323846264338327950288419716939937510582);
}

inline double func_arcsec(double num) { // https://en.neurochispas.com/calculators/arcsec-calculator-inverse-secant-degrees-and-radians/
    if (std::abs(num) < 1.0) {
        error(5);
        return 0.0;
    }
    return std::acos(1.0 / num); }

inline double func_arccsc(double num) { // https://en.neurochispas.com/calculators/arccsc-calculator-inverse-cosecant-degrees-and-radians/
    if (std::abs(num) < 1.0) {
        error(5);
        return 0.0;
    }
    return std::asin(1.0 / num);
}

inline double func_arccot(double num) { return 2 * std::atan(1) - std::atan(num); }

inline double func_sech(double num) {
    if (num > 88.02969) {
        error(5);
        return 0;
    }
    if (std::exp(num) + std::exp(-num) == 0) {
        error(5);
        return 0;
    }
    return 2 / (std::exp(num) + std::exp(-num));
}

inline double func_csch(double num) {
    if (num > 88.02969) {
        error(5);
        return 0;
    }
    if (std::exp(num) - std::exp(-num) == 0) {
        error(5);
        return 0;
    }
    return 2 / (std::exp(num) - std::exp(-num));
}

inline double func_coth(double num) {
    if (num > 44.014845) {
        error(5);
        return 0;
    }
    if (2 * std::exp(num) - 1 == 0) {
        error(5);
        return 0;
    }
    return 2 * std::exp(num) - 1;
}

inline double func_sec(double num) {
    if (std::cos(num) == 0) {
        error(5);
        return 0;
    }
    return 1 / std::cos(num);
}

inline double func_csc(double num) {
    if (std::sin(num) == 0) {
        error(5);
        return 0;
    }
    return 1 / std::sin(num);
}

inline double func_cot(double num) {
    if (std::tan(num) == 0) {
        error(5);
        return 0;
    }
    return 1 / std::tan(num);
}