#line 142 "bits.c"
int bitNor(int x, int y) {
  return (~x)&(~y);
}
#line 152
int isNotEqual(int x, int y) {

  return !(!(x^y));

}
#line 164
int anyOddBit(int x) {
 int a;int b;int c;int d;
 a = 0xaa << 8;
 b = a + 0xaa;
 c = b << 16;
 d = c + b;

 return !!(x & d);
}
#line 181
int rotateLeft(int x, int n) {
  int a;int b;int d;
  a = x << n;
  b = ~ 0x0;

  d = x >>( 33 + ~n);

  return a |( d &( ~(b << n)));
}
#line 197
int bitParity(int x) {
  int a;int b;int c;int d;int e;
  a = x ^( x >> 1);
  b = a ^( a >> 2);
  c = b ^( b >> 4);
  d = c ^( c >> 8);
  e = d ^( d >> 16);

  return e & 0x1;
}
#line 213
int tmin(void) {
  return 0x1 << 31;
}
#line 225
int fitsBits(int x, int n) {
  int a;
  a = 33 + ~n;
  return !(((x << a) >>a) ^ x);
}
#line 238
int rempwr2(int x, int n) {
    int a;int b;int c;
    a =( 1 << n) + ~0;
    b = x >> 31;
    c = x & a;

   return c +((( ~((!!c) << n)) + 1) & b);
}
#line 254
int addOK(int x, int y) {
  int a;int b;
  a = x + y;
  b =( x ^ a) &( y ^ a);
  return !((b >> 31) & 0x1);
}
#line 268
int isNonZero(int x) {
  return ((x|(~x+1)) >> 31) & 0x1;
}
#line 278
int ilog2(int x) {
    int mask1;int mask2;int mask3;int mask4;int r;int s;
    mask1 =(( 0xff << 8) + 0xff) << 16;
    mask2 = 0xff << 8;
    mask3 = 0xf0;
    mask4 = 0x0c;

    r = !!(x & mask1) << 4;
    x = x >> r;
    s = !!(x & mask2) << 3;
    x = x >> s;
    r = r + s;

    s = !!(x & mask3) << 2;
    x = x >> s;
    r = r + s;

    s = !!(x & mask4) << 1;
    x = x >> s;
    r = r + s;

    r = r +( x >> 1);

    return r;
}
#line 314
unsigned float_half(unsigned uf) {
  unsigned exp;unsigned sign;

  exp = uf & 0x7f800000;
  sign = uf & 0x80000000;
  if (exp == 0x7f800000) 
 return uf;
#line 324
  if (exp <= 0x800000) {
 if ((uf & 0x3) == 0x3) 
 return ((uf & 0xffffff) >> 1) + sign + 0x1;
 else 
 return ((uf & 0xffffff) >> 1) + sign;

}
return (uf & 0x807fffff) |( exp - 0x800000);
}
#line 344
unsigned float_twice(unsigned uf) {
  return 2;
}
