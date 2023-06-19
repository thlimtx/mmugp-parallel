| Bugs                                                                                             | Fix                                                        |
| ------------------------------------------------------------------------------------------------ | ---------------------------------------------------------- |
| max line does not count last line (does not have \n)                                             | maxline default = 1                                        |
| file reading does not reach the end of file (immediate and some others are missing from 1.txt)   | use line_num to check current read line for the whole file |
| chunk_size is 0 if lines <= np                                                                   |                                                            |
| word count is not accurate when using 1.txt, 2.txt, and 3.txt, but is accurate for smaller files |                                                            |

Limitations

- fgets does not have a built-in way of reading from a specific file, so another loop with a counter is needed, this slows down the process
