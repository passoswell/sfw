# Copilot Instructions

## Copyright Header (Hard Rule)

All `.cpp` and `.hpp` files must have the following line as the very first line of the file:

```cpp
// Copyright (c) 2026 sfw contributors. All rights reserved.
```

When creating or editing `.cpp` or `.hpp` files, always ensure this copyright line is present at the top.

## Use space instead of tab (Hard Rule)

Always use space instead of tab. One tab equals 2 spaces.

## Doxygen Parameter Direction (Hard Rule)

When generating or updating Doxygen-style comments for functions or methods, always specify parameter direction explicitly:
- Use `@param[in]` for input parameters.
- Use `@param[out]` for output parameters.
- Use `@param[in,out]` for parameters used for both input and output.

Do not use bare `@param` without a direction tag.

## Doxygen Return Value Tag (Hard Rule)

When generating or updating Doxygen-style comments for functions or methods, use `@retval` instead of `@return`.

- Use one `@retval` entry per documented return value.
- Do not use `@return`.

## Doxygen Destructor Brief Tag (Hard Rule)

When generating or updating Doxygen-style comments for destructors, the `@brief` text must be exactly:
- `Destructor`

Do not use alternative destructor brief text.

## Doxygen comments must respect the line length limit (Hard Rule)

When generating or updating Doxygen-style comments, ensure that no line exceeds
80 characters in length. This includes the comment delimiters and any
indentation. Verify if the project configuration specifies a different line
length limit and adhere to that if so.

## Doxygen comments for classes must include the Typical usage section (Hard Rule)

When generating or updating Doxygen-style comments for classes, include a "Typical usage" section that provides a clear, step-by-step example of how to use the class. This section should be formatted as a numbered list and should cover the common use cases for the class.

## Doxygen Include Paths for New Classes (Hard Rule)

When generating a new class or moving a header, keep the documented include
path repository-relative instead of a bare filename. Prefer includes like
`#include <hal_interface/...>` or `#include <hal_linux/...>` for classes under
`libraries/`, and make sure the Doxyfile `STRIP_FROM_INC_PATH` and
`INCLUDE_PATH` settings include the corresponding header roots so Doxygen shows
the intended path in the generated class documentation.

## Doxygen comments for abstract methods must specify expected behavior (Hard Rule)

When generating or updating Doxygen-style comments for abstract methods, ensure that the comments clearly specify the expected behavior of the method. This includes describing how the method should be implemented by concrete subclasses, any assumptions or requirements for the implementation, and any important details about how the method interacts with other parts of the class or system. The comments should provide enough information for a developer to understand how to correctly implement the method in a subclass.

## Doxygen comments for abstract methods must specify call dependencies (Hard Rule)

When generating or updating Doxygen-style comments for abstract methods, ensure that the comments clearly specify any call dependencies. This includes describing any other methods that must be called before or after the abstract method, any required initialization steps, and any expected interactions with other methods or components of the class. The comments should provide a clear understanding of how the abstract method fits into the overall workflow of the class and what other methods or actions are necessary for its correct usage.

## Doxygen comments for methods that implement abstract methods must specify implementation details (Hard Rule)

When generating or updating Doxygen-style comments for methods that implement abstract methods, ensure that the comments clearly specify the implementation details. This includes describing how the method fulfills the contract defined by the abstract method, any specific logic or algorithms used in the implementation, and any important considerations or edge cases that were addressed. The comments should provide insight into how the method works and why certain design choices were made in the implementation. In addition, it is important to describe any deviations from the abstract method's contract or any additional behavior provided by the implementation.