//vec2 v;
//float u;

float v;
int a;


float fortest()
{
  int i;

  float sum;
  a = 5;
  
  sum = v;
  for ( i = 0; i < a; i += 1 )
    sum *= 2.0;

  return sum;
}


/*
void fortest()
{
  int i;

  float sum;
  a = 5;
  
  sum = v;
  for ( i = 0; i < a; i += 1 )
    sum *= 2.0;

  return;
  //return sum;
}
*/

/*
vec2 v;
float fortest (int a)
{
	int i;
	float sum;
	sum = v.x;
	for ( i = 0; i < a; i += 1 )
		sum *= v.y;
	return sum;
}
*/

