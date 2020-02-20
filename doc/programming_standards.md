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
`i`, `j`, `k`, and `l` are fairly standard. Algorithms based matrix interpretation
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
