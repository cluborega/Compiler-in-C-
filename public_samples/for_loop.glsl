//vec2 v;
float u;

float fortest(int a)
{
  int i;
  //float sum;
  float sum = 1.0;
  
  u = 10.0;
  //sum = v.x;
  for ( i = 0; i < a; i += 1 )
    //sum *= v.y;
	sum *= u;
	
  return sum;
}
