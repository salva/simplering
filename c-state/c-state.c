
#include <stdio.h>
#include <stdlib.h>

#define in_state(s) state_ ## s: case s
#define jump(var, s) do { var = s; goto state_ ## s ;} while(0)

enum states { A, B, C };

int
main() {
    int state = B;
    switch(state) {
    in_state(A):
        printf("in state A\n");
        jump(state, C);

    in_state(B):
        printf("in state B\n");
        jump(state, A);

    in_state(C):
        printf("in state C\n");
        exit(0);
    }
}
