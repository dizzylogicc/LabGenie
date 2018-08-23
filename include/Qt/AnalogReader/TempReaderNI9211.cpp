/* Copyright (c) 2018 Peter Kondratyuk. All Rights Reserved.
*
* You may use, distribute and modify the code in this file under the terms of the MIT License, however
* if this file is included as part of a larger project, the project as a whole may be distributed under a different
* license.
*
* MIT license:
* Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
* documentation files (the "Software"), to deal in the Software without restriction, including without limitation
* the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and
* to permit persons to whom the Software is furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all copies or substantial portions
* of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
* TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
* THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
* CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
* IN THE SOFTWARE.
*/

#ifdef WITH_NI_HARDWARE

#include "HwNI9211.h"
#include "TempReaderNI9211.h"


double TempReaderNI9211::Period()
{
	return 1.0 / hwNI9211->SampleRate();
}

bool TempReaderNI9211::Initialize(StdMap<BString, ExpDevice*>& devMap)
{
	//Try to cast the hardware name to HwNI9211 and return false is failed
	HwNI9211* dev = dynamic_cast<HwNI9211*>(devMap[hardware]);
	if (!dev) return false;

	//Everything's OK - add the reader to the hardware
	dev->AddReader(this);
	SetHardware(dev);
	return true;
}

#endif //WITH_NI_HARDWARE