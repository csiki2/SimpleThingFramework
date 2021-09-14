# Coding style

## Naming conventions

### Variables
- All local variable should be **camelCase** and should not contain underscore
- All global variable should be **PascalCase** with a g_ prefix
- There should be no global function (not even in the stf namespace), should be added as a static class method

### Classes
- All class name should be **PascalCase**
- All method name should be **camelCase** (static "method" as well)
- All member variable should be **camelCase** (static "member" as well) with an **underscore** prefix (`_`)

### Struct
- Struct should be used where POD initialization is favored; struct should have no constructors
- All struct name should be **PascalCase**
- All method name should be **camelCase** (static "method" as well)

### Enum
- All enum type name should be **PascalCase**
- All enum type should be **enum class**
- TODO: enum name format


## Coding format

- Use Visual Studio Code - Clang-Format extension (alt-shift-f)
