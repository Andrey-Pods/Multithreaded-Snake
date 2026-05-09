# Mutation Engine Specification

## List of Mutations

Provided here is the list of mutations. Most of these mutations will 
be reliant on the `hash` function declared in `global.h`. These
mutations do not modify the input provided as an argument to the `mutate`
function. There is also a maximum input length of $1024$.

The following notation will be used throughout:

Let the length of the input be $N$. Let $\text{HASH}$ be the hash 
function. Define the following sequence $H_{n,m}$ of functions as 
follows:

$$
\begin{aligned}
H_{n,0}(x) &= x \\
H_{n,m}(x) &= \text{HASH}(H_{n,m-1}(x))\mod n 
\end{aligned}
$$

Let $M$ be the total number of mutations in a cycle. Let $S$ be the 
current mutation state. Define $K=\lfloor S/M\rfloor+1$ be the 
cycle index. Finally, given string $A$, let $A[i]$ represent the 
$i$th character in the string $A$. This is zero-indexed.

### [1] Fill With Single Character To Length

This mutation strategy may lengthen or shorten an input. If the input
is lengthened, then the added characters are filled with the character
'a'. If the input is shortened, the input is truncated to the target
length. The target length is $2^{K}$. If the input is the empty string,
however, return a new input object holding an empty string.

### [2] Duplicate Input $K+1$ times

This mutation duplicates the input such that the final input length
is $K+1$ times the original input. The only exception is if the maximum
input length is exceeded. In this case, the input is truncated to the
maximum. The input is copied to fill the new length. If the input is
the empty string, return a new input object holding an empty string. 

### [3] Lengthen String By $L$ Characters

This mutation returns an input for a string of length $N+L$. The
input string is copied into the first $N$ characters of the mutated
string. Let the mutated string be $A$. Then for $i$ from $0$ to $L-1$
inclusive, the character at $A[N+i]$ is $A[H_{N,i+1}(S)]$. If the input
is the empty string, return a new input object holding an empty string.

### [4] Truncate Input

This mutation truncates the input to length $H_{N,1}(S)$. If the input
is the empty string, return a new input object holding an empty string.

### [5] Inject String

This mutation selects $p=(K+1)/2$ locations within the input string to inject
a string $A$. This mutation does not increase the length of the string, and
it will truncate the injected string so that the length of input does not
change. The starting locations to inject the string are at $H_{N,i}(S)$ 
for $i$ in range $1$ to $p$ inclusive. If the input is the empty string,
return a new input object holding an empty string.

### [6] Inject Random Integer String

This mutation selects $p=(K+1)/2$ locations within the input string to inject
a string of a random 32-bit signed integer $Z$. This random 32-bit integer is 
chosen to be $\text{HASH}(S)$ casted to being a 32-bit signed integer. This
mutation does not increase the length of the string, and it will truncate the 
injected string so that the length of input does not change. The starting locations
to inject the string are at $H_{N,i}(S)$ for $i$ in range $1$ to $p$ inclusive.
If the input is the empty string, return a new input object holding an empty string.

### [7] Inject Random Substring of Length $L$

This mutation selects $p=(K+1)/2$ locations within the input string to inject
a substring of length $L$. The substring of length $L$ begins at $H_{N,1}(S)$.
If $H_{N,1}(S)+L$ is greater than $N$, then the substring terminates when the
input string terminates. 
This mutation does not increase the length of the string, and it will truncate
the injected string so that the length of the input does not change. It should also not decrease the size of the input. The
starting locations to inject the substring are at $H_{N,i+1}(S)$ for $i$ in
range $1$ to $p$ inclusive. If the input is the empty string, return a new input
object holding an empty string.

### [8] Inject Character

This mutation selects $p=(K+1)/2$ locations within the input string to inject
a specific character $C$. The locations to substitute with the character $C$
is $H_{N,i}(S)$ for $i$ in range $1$ to $p$ inclusive. If the input is the empty
string, return a new input object holding an empty string.

### [9] Flip $L$ Bits

This mutation selects $p=(K+1)/2$ bit locations within the input string to flip
$L$ bit seqeuences of. The locations to flip the sequence of $L$ bits are at $H_{8\cdot N, i}(S)$
for $i$ in range $1$ to $p$ inclusive. Bit location refer to the position of a single
bit in the string, meaning there are $8\cdot N$ bit locations in the input string.
Do not flip bits outside outside the range of bit locations in the input string. 
If the input is the empty string, return a new input object holding an empty string.

### [10] Increment Bytes

This mutation selects $p=(K+1)/2$ byte locations. At these locations, the mutation
simply increment the byte by one. The locations to increment are at $H_{N,i}(S)$
for $i$ in range $1$ to $p$ inclusive. If the input string is the empty string,
return a new input object holding an empty string.

### [11] Decrement Bytes

Similar to [10] Increment Bytes except for it now decrements by one instead of
incrementing. Similarly, if the input is the empty string, return a new input
object holding an empty string.