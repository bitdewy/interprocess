::  Copyright 2013, bitdewy@gmail.com
::  Distributed under the Boost Software License, Version 1.0.
::  You may obtain a copy of the License at
::
::  http://www.boost.org/LICENSE_1_0.txt

@echo off
for /R %%s in (*.h,*.cpp) do ( 
  python cpplint\cpplint.py %%s 
)
pause
