#ifdef DEBUG
#include <iostream>
using namespace std;
#endif

int fib(int n) {
    if (n < 0) return 0;
    if (n == 0 || n == 1) return 1;
    return (fib(n - 1) + fib(n - 2)) & 0xffff;
}

const int arr[16] = {177, -13012, 9, 80, -14134854, 7777, 9142, -3, 12, 2047, 93, 486, 11, 22, 33, 44};
int check[6] = {0};
int ans = 0;

int main(void) {

    ans += fib(arr[8]) << 6;
#ifdef DEBUG
    cout << ans << endl;
#endif
    check[0] = ans;
    if (ans & (1 << arr[2])) {
        ans -= arr[1];
        if (ans > -arr[1] + arr[5] * 2) {
            ans >>= 3;
        } else {
            ans >>= 4;
        }
        ans <<= 7;
        ans *= (-11);
        ans <<= 1;
#ifdef DEBUG
        cout << ans << endl;
#endif
        check[1] = ans;
    }

    for (int i = 0; i < arr[0]; i++) {
        ans >>= 3;
        ans = (int)((unsigned int)ans >> 3);
        ans <<= 7;
        ans ^= 0xffffffff;
    }
#ifdef DEBUG
    cout << ans << endl;
#endif
    check[2] = ans;

    ans &= 0xffff;
    ans |= 0x1234;
    ans |= 0x9090;

    ans += 11112;
    ans -= arr[9] * arr[10];

#ifdef DEBUG
    cout << ans << endl;
#endif
    check[3] = ans;

    for (int i = 0; i < ((arr[6] + 1) & 0xfff); i++) {
        ans *= ans;
        if (ans >= 0xffff) {
            ans &= 0xffff;
        }
        ans += arr[(i * ans * arr[(i + 3) & 0xf]) & 0xf];
    }

#ifdef DEBUG
    cout << ans << endl;
#endif
    check[4] = ans;

    ans <<= 7;
    ans &= 0xffffff;
#ifdef DEBUG
    cout << ans << endl;
#endif
    check[5] = ans;

	return 0;
}

// 14912
// -4913920
// 1431655807
// -123964
// -127072575
// 8609920
