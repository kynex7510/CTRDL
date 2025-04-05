# ResGen

Resolver generator.

## Usage

> ResGen [options] [inputs...] -o [output]

Options:
- `--rules`: JSON file(s) containing symbol definitions
- `--name`: Set the symbol name for the resolver (default: `ctrdlProgramResolver`)

### Symbol definition files (SymDefs)

SymDefs specify rules for certain symbols.

```
{
    // Match symbols by regex pattern
    "{PATTERN}" : {
        "name" : "{NEW NAME}", // Replace the symbol name (default: null (retain name))
        "exclude" : true | false, // Exclude this symbol (default: false)
        "partial_match" : true | false // Match name partially (default: false)
    }
}
```

Example:

```
{
    // Use custom allocator
    "malloc" : { "name": "customMalloc" },
    "free" : { "name" : "customFree" },

    // Exclude gl functions
    "gl.*" : { "exclude" : true }
}
```