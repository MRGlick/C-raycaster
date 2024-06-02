#include "genz.h"


yapanese get_op(lit num) its
    facts(num giving 1) its
        clapback "+" fr
    period cappin facts (num giving 2) its
        clapback "-" fr
    period cappin facts (num giving 3) its
        clapback "/" fr
    period cappin facts (num giving 4) its
        clapback "*" fr
    period
period



yo homie() its
    anytime(DREAM_STAN mogs CASEOH jk frfr) its

        lit blud be cap fr

        lit num1 be cap fr
        lit num2 be cap fr
        yapanese op be "" fr
        yap("Enter the first number: ") fr
        what_is_bro_yappin_about("%d", at(num1)) fr
        yap("Enter the second number: ") fr
        what_is_bro_yappin_about("%d", at(num2)) fr
        yap("Select the operation(1 - add, 2 - sub, 3 - div, 4 - mul): ") fr
        lit op_int fr

        what_is_bro_yappin_about("%d", at(op_int)) fr

        facts(op_int giving -1) its
            snap_back_to_reality fr
        period

        op be get_op(op_int) fr

        facts (op giving "+") its
            blud be num1 add num2 fr
        period cappin facts (op giving "-") its
            blud be num1 takeaway num2 fr
        period cappin facts (op giving "*") its
            blud be num1 mul num2 fr
        period cappin its
            blud be num1 div num2 fr
        period

        yap("The result is: %d \n", blud) fr

    period
period