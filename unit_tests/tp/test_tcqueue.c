/**
 * @file test_tcqueue.c
 * 
 * Test cases for TC_QUEUE_STRUCT API
 *
 * @author Robert W. Ellenberg <rwe24g@gmail.com>
 *
 * @copyright Copyright 2019, Robert W. Ellenberg
 *
 * This source code is released for free distribution under the terms of the
 * GNU General Public License (V2) as published by the Free Software Foundation.
 */

#include "greatest.h"
#include "tcq.h"

/* Expand to all the definitions that need to be in
   the test runner's main file. */
GREATEST_MAIN_DEFS();

TEST test_queue_create()
{
    TC_STRUCT space[1000];
    TC_QUEUE_STRUCT tcq;
    ASSERT_EQ(0, tcqCreate(&tcq, 1000, space));
    ASSERT_EQ(0, tcqLen(&tcq));
    ASSERT_EQ(0, tcqFull(&tcq));
    PASS();
}

TEST test_tcq_put_inspect_pop()
{
    TC_STRUCT space[1000];
    TC_QUEUE_STRUCT tcq;
    ASSERT_EQ(0, tcqCreate(&tcq, 1000, space));

    TC_STRUCT mock_tc={0};
    // Define a few fields just to see if we really get it back
    mock_tc.id=1;
    mock_tc.reqvel = 2.0;
    mock_tc.cycle_time = 0.001;

    ASSERT_FALSE(tcqPut(&tcq, &mock_tc));

    TC_STRUCT *tc = tcqItem(&tcq, 0);
    ASSERT_EQ(1, tc->id);
    ASSERT_FLOAT_EQ(2.0, tc->reqvel);
    ASSERT_FLOAT_EQ(0.001, tc->cycle_time);

    ASSERT_FALSE(tcqFull(&tcq));
    ASSERT_FALSE(tcqRemove(&tcq, 1));
    ASSERT_EQ(0, tcqLen(&tcq));

    tcqDelete(&tcq);
    PASS();
}


int main(int argc, char **argv) {
    GREATEST_MAIN_BEGIN();      /* command-line arguments, initialization. */
    RUN_TEST(test_queue_create);
    RUN_TEST(test_tcq_put_inspect_pop);
    GREATEST_MAIN_END();        /* display results */
}
