# Basic Features
Recursive descent parser that builds expression AST. Everything evaluates to an expression which evaluates to a double. 

1. Supports `+, -, *, /` as well as 
2. Variable assignment can be done by `a=2` or `a=b=2`.
3. Logical operations like `&&`, `||`
4. Comparison with `==`
5. Function definition with `fn fnname(arg1, arg2, ...) { expr1; expr2; returnexpr } 
6. Ternary operator `cond ? iftrue : iffalse`
7. `Ans` automatic variable to get previous answer
8. Builtin functions: `sin(x)`. 

# Examples

### Simple operations

```
-4+1*7+2/3
3.666667
```

### Floating point numbers
```
355/113
3.141593
```

### Variables 
```
> a=b=2
> a+b
4
```

### Logical Operations
```
> 1 && 2 && 3
1
> 1 && 0
0
> 1 || 0
1
```

### Functions
```
Last expression is implicitly returned

> fn fib(n) {n==0 || n==1 ? n : fib(n-2)+fib(n-1) }
> fib(10)
55
```


Todo:
- Units
