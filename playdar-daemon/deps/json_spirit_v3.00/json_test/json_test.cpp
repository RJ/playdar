/* Copyright (c) 2007-2009 John W Wilkinson

   json spirit version 3.00

   This source code can be used for any purpose as long as
   this comment is retained. */

#include "json_spirit_value_test.h"
#include "json_spirit_writer_test.h"
#include "json_spirit_reader_test.h"
#include "json_spirit_utils_test.h"
#include "json_spirit.h"

#include <string>
#include <iostream>

using namespace std;
using namespace json_spirit;

int main()
{
    test_value();
    test_writer();
    test_reader();
    test_utils();

    cout << "all tests passed" << endl << endl;
    cout << "press any key to continue";

    string s;
    cin >> s;

	return 0;
}

