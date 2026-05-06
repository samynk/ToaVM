class LoopTest
{
	int offset;

	void testFor(){
		int sum =0;
		for(int i = 0; i <10; ++i)
		{
			sum += i;
		}
	}

	int sum(int a, int b){
		return a+b+offset;
	}

	int sum(int a, int b, int c){
		return a+b+c+offset;
	}
}
