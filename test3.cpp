#define __is_consteval_only(x) false
#include <meta>
constexpr auto info = ^int;
int main() { return 0; }
