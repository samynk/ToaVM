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

float angle(float y, float x)
{
    return atan2(y,x);
}

int sum()
{
    int op1 = 2;
    int op2 = 5;
    return op1 + op2;
}

/*
int run_demo()
{
    int answer = square(6) + square(8);
    print(answer);
    return answer;
}
*/
