bench
=====

A tiny very simple benchmarking tool.

    Usage: bench [OPTIONS] [--] COMMAND...
    
    OPTIONS:
    
            -h, --help             Print this help message.
            -t, --times=N          Repeat command N times. (default: 100)

Prints output like this:

    runs:       100
    sum:          1.862160810 sec
    minimum:      0.015729335 sec
    maximum:      0.022924934 sec
    average:      0.018621608 sec
    median:       0.018263226 sec

MIT License
-----------

Copyright 2020 Mathias Panzenböck

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
