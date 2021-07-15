# tips

* Follow standard conventions.
* Keep it simple stupid. Simpler is always better. Reduce complexity as much as possible.
* Boy scout rule. Leave the campground cleaner than you found it.
* Always find root cause. Always look for the root cause of a problem.

* Use meaningful and pronounceable variable names
* Use searchable names
* Don't use flags as function parameters
* Functions should do one thing
* Don't add unneeded context
* Avoid positional markers
* Don't have journal comments

## fix compiler warnings

```c
/// Add header 
#include <stddef.h>
/// Replase 
/// #define MESSWRELATIV_OFFSET(messw) ((UINT) & (((STU_PROT_MSWERTE_EXTD *)NULL)->messw))
#define MESSWRELATIV_OFFSET(messw) offsetof(STU_PROT_MSWERTE_EXTD,messw)
 ```


## Git messages example
* type(category1,category2): message #issue
* fix: typo in README.md
* fix: X to allow Y to use Z
* fix: failing CompositePropertySourceTests
* add: tests for ImportSelector meta-data
* enhancement: Add support for instance accounts
* chore: Run `make generate`
* chore: Prepare v0.8.2 release
* docs: remove now obsolete credits section
* build: compile on host meson