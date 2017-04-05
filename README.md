# mch
Highlight Perl regular expression patterns with multiple colors

![Alt Text](https://github.com/dczhu/mch/blob/master/res/h.gif)

## Introduction
This was initially from [hhighlighter](https://github.com/paoloantinori/hhighlighter), but I decided to implement the low level pattern parsing and highlighting, which are handled by ack/ack-grep in hhighlighter. This is because I found issues when using hhighlighter in some test cases - For example:

- [ ] ```echo "tastb'c" | h "b\'c"``` will reach the following error:
  > bash: eval: line 145: unexpected EOF while looking for matching `'
  > bash: eval: line 146: syntax error: unexpected end of file
- [ ] ```echo "abbcccdddd" | h 'b?'``` won't come back to shell until Ctrl+C.
- [ ] ```echo "abcdefgababcde" | h '(ab){2,5}'``` will highlight only the last "ab", not "abab".

By using mch, all these problems go away, just like how ```grep -P``` does in pattern matching - Besides, mch supports different colors for different patterns. That's it.

## Installation
In ~/.bashrc, source the script [h](https://github.com/dczhu/mch/blob/master/h). And the command ```h``` will be ready for use.

## Usage
The text that has patterns to be highlighted **has to** be piped into the command ```h```. It may have the following use cases:

* Used by other scripts, like [cxpgrep](https://github.com/dczhu/cxpgrep/blob/master/cxpgrep), in output processing.
* Highlight patterns in a text file in different colors: ```cat FILE | h PAT1 PAT2 ... | less -R```

It supports only 1 option -i that means case insensitive matching.

## Meta

Deng-Cheng Zhu (dengcheng _DOT_ zhu _AT_ gmail _DOT_ com)

Distributed under the MIT license. See LICENSE for more information.

https://github.com/dczhu/
