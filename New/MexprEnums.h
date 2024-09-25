#pragma once

typedef enum {
	MATH_CPP_INT,
	MATH_CPP_DOUBLE,
	MATH_CPP_VARIABLE,  // alphanumeric word - "Abhishek"
	MATH_CPP_IPV4,  // 10.1.1.1
	MATH_CPP_DTYPE_MAX
} mexprcpp_dtypes_t;

typedef enum {
	PARSER_EOL = (int)MATH_CPP_DTYPE_MAX + 1
} EXTRA_ENUMS;
