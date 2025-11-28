/*
 * Copyright (C) 2009-2014 Francesco Nidito
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
/* For the full documentation and explanation of the code below, please refer to
 * http://www.di.unipi.it/~nids/docs/longjump_try_trow_catch.html
 */
/*
 * Modified 2025 AESilky
 *
 * Make it compliant with the C standard by changing `ETRY` to `Etry`, as identifiers
 * or macros starting with capital E and followed by another capital letter or number
 * are reserved for use by the standard <errno.h> include file. To be consistent, all
 * the macros have been changed to mixed case.
 *
 * In addition, a base Exception and an example have been added.
 */

#ifndef TRY_THROW_CATCH_H_
#define TRY_THROW_CATCH_H_

#include <setjmp.h>

extern jmp_buf ex_buf__;
#define TryCatchInit jmp_buf ex_buf__

#define Try do { jmp_buf ex_buf__; switch( setjmp(ex_buf__) ) { case 0: while(1) {
#define Catch(x) break; case x:
#define Finally break; } default: {
#define Etry break; } } } while(0)
#define Throw(x) longjmp(ex_buf__, x)

#define EXCEPTION_GENERAL (1000)

#endif /*!TRY_THROW_CATCH_H_*/

/*
Example:

#define FOO_EXCEPTION (1)
#define BAR_EXCEPTION (2)
#define BAZ_EXCEPTION (3)

int
main(int argc, char** argv)
{
    Try
    {
        printf("In Try Statement\n");
        Throw( BAR_EXCEPTION );
        printf("I do not appear\n");
    }
    Catch( FOO_EXCEPTION )
    {
        printf("Got Foo!\n");
    }
    Catch( BAR_EXCEPTION )
    {
        printf("Got Bar!\n");
    }
    Catch( BAZ_EXCEPTION )
    {
        printf("Got Baz!\n");
    }
    Finally
    {
        printf("...et in arcadia Ego\n");
    }
    Etry;

    return 0;
}


Output:
    In Try Statement
    Got Bar!
    ...et in arcadia Ego

*/