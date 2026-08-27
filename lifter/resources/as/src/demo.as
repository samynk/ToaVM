int square(int value)
{
    return value * value;
}

float squarehypotenuse(float a, float b)
{
    return a*a + b*b;
}

float hypotenuse(float a, float b)
{
    return sqrt(a*a + b*b);
}

int run_demo()
{
    int answer = square(6) + square(8);
    print(answer);
    return answer;
}



