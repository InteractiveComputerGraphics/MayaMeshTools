add_library(mzd STATIC
	readMZD.cpp
	readMZD.h
	readMZD_half2float.h
)

set_target_properties(mzd PROPERTIES FOLDER "External Dependencies")
 


