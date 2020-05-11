# Standards for Programming

## Source Documentation

The `ngen` project will be using Doxygen for documentation generation. Initially, we'll
follow the Javadoc C-style standard. This means we'll see documentation like:

```
/**
 *  A test class. A more elaborate class description.
 */
 
class Javadoc_Test
{
  public:
 
    /** 
     * An enum.
     * More detailed enum description.
     */
 
    enum TEnum { 
          TVal1, 
          TVal2,
          TVal3
         } 
       *enumPtr,
       enumVar;
       
      /**
       * A constructor.
       * A more elaborate description of the constructor.
       */
      Javadoc_Test();
 
      /**
       * A destructor.
       * A more elaborate description of the destructor.
       */
     ~Javadoc_Test();
    
      /**
       * a normal member taking two arguments and returning an integer value.
       * @param a an integer argument.
       * @param s a constant character pointer.
       * @see Javadoc_Test()
       * @see ~Javadoc_Test()
       * @see testMeToo()
       * @see publicVar()
       * @return The test results
       */
       int testMe(int a,const char *s);
       
      /**
       * A pure virtual member.
       * @see testMe()
       * @param c1 the first argument.
       * @param c2 the second argument.
       */
       virtual void testMeToo(char c1,char c2) = 0;
   
      /** 
       * a public variable.
       */
       int publicVar;
       
      /**
       * a function variable.
       * @param a the first argument.
       * @param b the second argument.
       */
       int (*handler)(int a,int b);
};
```

Each class should have comments detailing what it's used for and why. Each function and method
should also have what those are used for, why, and descriptions of their parameters.
Comments should be dotted throughout functions for complex lines describing 
intent rather than implementation.

## Naming

Variable names should be descriptive, yet concise nouns.  While a lay person may not
understand that what and why of the source, they should be able to glean a rough idea 
as to what the implementation is.

#### Bad Variable Names:

```
int x;
int variableForDeterminingSomeMathThingIDoNotKnowThatCalculates;
int px;
```

Control structures should _also_ have descriptive names, even though variables like
`i`, `j`, `k`, and `l` are fairly standard. Algorithms based on matrix interpretation
become very difficult to read with short names. Take a longest common subsequence example:

```
int max(int a, int b); 
  
/* Returns length of LCS for X[0..m-1], Y[0..n-1] */
int lcs(char* X, char* Y, int m, int n) 
{ 
    int L[m + 1][n + 1]; 
    int i, j; 
  
    /* Following steps build L[m+1][n+1] in bottom up fashion. Note  
      that L[i][j] contains length of LCS of X[0..i-1] and Y[0..j-1] */
    for (i = 0; i <= m; i++) { 
        for (j = 0; j <= n; j++) { 
            if (i == 0 || j == 0) 
                L[i][j] = 0; 
  
            else if (X[i - 1] == Y[j - 1]) 
                L[i][j] = L[i - 1][j - 1] + 1; 
  
            else
                L[i][j] = max(L[i - 1][j], L[i][j - 1]); 
        } 
    } 
  
    /* L[m][n] contains length of LCS for X[0..n-1] and Y[0..m-1] */
    return L[m][n]; 
} 
  
/* Utility function to get max of 2 integers */
int max(int a, int b) 
{ 
    return (a > b) ? a : b; 
} 
```

On first glance, it is difficult to make out:

a) What the function is supposed to be doing (what does `lcs` stand for?)
b) How exactly `X` and `Y` are meant to be used
c) What `i` is supposed to be indexing
d) What `j` is supposed to be indexing
c) What is supposed to be stored in `L`

#### General Guidelines

When in doubt, follow the example given by C++ and Boost (though it would
be prudent to stray away from some of their names like `point_t` that might
hide intent). That means that most `class`, `variable`, `function`, and
`parameter` names will be in snake case. Examples of snake case are
`regex_search`, `forward_list`, and `variable_name`. `enum`s, however, should
be represented with a leading upper case, like `Web_color`. Macros and
template parameter names should always be defined as `ALL_UPPER_CASE`.

In the end, err on the side of readability; code that strays from the
standard, yet is easy to read is more valuable than code that sticks to
the letter of the law, yet is hard to work with.

## Other

### Headers

**ALWAYS** make sure that you guard your headers as such:

    #ifndef EXAMPLE_H
    #define EXAMPLE_H
    .
    .
    .
    #endif // EXAMPLE_H

Make sure to leave the comment at the end if the file is long, just as a
common courtesy.

### Spaces vs Tabs

Use spaces in lieu of tabs. Tabs are easier to type, but may cause code to
become very difficult to read on systems with odd spacing. If you tab your code a couple times, it
may look fine on your machine, but code like

```
int max(int a, int b); 
  
/* Returns length of LCS for X[0..m-1], Y[0..n-1] */
int lcs(char* X, char* Y, int m, int n) 
{ 
    int L[m + 1][n + 1]; 
    int i, j; 
  
    /* Following steps build L[m+1][n+1] in bottom up fashion. Note  
      that L[i][j] contains length of LCS of X[0..i-1] and Y[0..j-1] */
    for (i = 0; i <= m; i++) { 
        for (j = 0; j <= n; j++) { 
            if (i == 0 || j == 0) 
                L[i][j] = 0; 
  
            else if (X[i - 1] == Y[j - 1]) 
                L[i][j] = L[i - 1][j - 1] + 1; 
  
            else
                L[i][j] = max(L[i - 1][j], L[i][j - 1]); 
        } 
    } 
  
    /* L[m][n] contains length of LCS for X[0..n-1] and Y[0..m-1] */
    return L[m][n]; 
} 
```

might look like the following when moved to a different machine:

```
int max(int a, int b); 
  
/* Returns length of LCS for X[0..m-1], Y[0..n-1] */
int lcs(char* X, char* Y, int m, int n) 
{ 
        int L[m + 1][n + 1]; 
        int i, j; 
  
        /* Following steps build L[m+1][n+1] in bottom up fashion. Note  
           that L[i][j] contains length of LCS of X[0..i-1] and Y[0..j-1] */
        for (i = 0; i <= m; i++) { 
                for (j = 0; j <= n; j++) { 
                        if (i == 0 || j == 0) 
                                L[i][j] = 0; 
  
                        else if (X[i - 1] == Y[j - 1]) 
                                L[i][j] = L[i - 1][j - 1] + 1; 
  
                        else
                                L[i][j] = max(L[i - 1][j], L[i][j - 1]); 
                } 
        } 
  
        /* L[m][n] contains length of LCS for X[0..n-1] and Y[0..m-1] */
        return L[m][n]; 
} 
```
`\t`, as entered by the 'tab' key, is generally rendered as four consecutive
spaces. Before you start writing, you should ensure that your editor of
choice interprets the 'tab' key as four spaces. Most, if not all, editors
support this.

In VIM, for example, you can set the tab key to insert spaces via:

    set tabstop=4 shiftwidth=4 expandtab

Editors like Visual Studio Code are already set to 
convert tabs to a language appropriate number of spaces.

### Braces

Bracing should follow either the classical Kernighan and Ritchie
style or it's popuplar offshoot, the 'One True Brace Style'.

A block of K&R will look like:

```
int main(int argc, char *argv[])
{
    ...
    while (x == y) {
        something();
        somethingelse();

        if (some_error)
            do_correct();
        else
            continue_as_usual();
    }

    finalthing();
    ...
}
```
The opening brace for the outer level element lies on a
newline while the rest of the opening braces appear
on the same line. All closing braces appear on a new line
at the same indention as its parent. Braces around single
line functions in control statements are optional.

That same block in 'One True Brace Style' looks like:
```
int main(int argc, char *argv[]) {
    ...
    while (x == y) {
        something();
        somethingelse();

        if (some_error) {
            do_correct();
        } else {
            continue_as_usual();
        }
    }

    finalthing();
    ...
}
```
This is identical to K&R in all but two respects: opening
braces are always on the same line and control statements
**always** have braces.

### Line Width

Lines should be kept at or less than 120 characters.
