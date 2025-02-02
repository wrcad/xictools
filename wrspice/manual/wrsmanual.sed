\documentclass[twoside,fleqn]{report}
\makeindex
\usepackage{latexsym,longtable,epsfig,makeidx,tocloft}
\pagestyle{headings}
\setcounter{tocdepth}{5}
\setcounter{secnumdepth}{5}
\setlength{\topmargin}{0in}
\setlength{\textheight}{8.5in}
\setlength{\footskip}{.5in}
\setlength{\oddsidemargin}{.25in}
\setlength{\evensidemargin}{-.25in}
\setlength{\textwidth}{6.25in}
\setlength{\parskip}{1.5ex}
\setlength{\mathindent}{1cm}
\setcounter{tocdepth}{3}
\newenvironment{lquote}{\begin{list}{}{}\item[]}{\end{list}}
\newcommand{\rr}{\raggedright}
\newcommand{\Xic}{\sf\slshape Xic}
\newcommand{\XicII}{\sf\slshape XicII}
\newcommand{\Xiv}{\sf\slshape Xiv}
\newcommand{\XicTools}{\sf\slshape XicTools}
\newcommand{\WRspice}{\sf\slshape WRspice}
\newcommand{\spgen}[1]{\begin{description}\item{General Form:}\\{\tt #1}
 \end{description}}
\newcommand{\kb}{\sf\bfseries }  %keyboard keys
\newcommand{\cb}{\bf } %command buttons
\newcommand{\et}{\sf } %emphasized text
\newcommand{\vt}{\tt } %verbatim text
\newcommand{\spexamp}[1]{\begin{description}\item{Examples:}\\{\tt #1}
 \end{description}}
\newcommand{\spexampo}[1]{\begin{description}\item{Example:}\\{\tt #1}
 \end{description}}
\newcommand{\kbkey}{\sf\bfseries}
\setlongtables

%
% Unfortunately, latex2html doesn't handle conditionals, so these are
% hard coded and explicitly commented, for now.
%
% Whether or not to include license server references.
%\newif\ifxtlserv
%\xtlservfalse

% Have to modify the TOC number spacing.  The following lines do this
% without tocloft, but cause trouble in latex2html
%
% \makeatletter
%\renewcommand*\l@section{\@dottedtocline{2}{1.5em}{3.3em}}
%\renewcommand*\l@subsection{\@dottedtocline{3}{4.8em}{4.2em}}
%\renewcommand*\l@subsubsection{\@dottedtocline{4}{9.0em}{4.1em}}
% \makeatother

% \setlength{\cftchapindent}{0em}
% \setlength{\cftchapnumwidth}{1.5em}
\setlength{\cftsecindent}{1.5em}
\setlength{\cftsecnumwidth}{3.3em}
\setlength{\cftsubsecindent}{4.8em}
\setlength{\cftsubsecnumwidth}{4.2em}
\setlength{\cftsubsubsecindent}{10.5em}
\setlength{\cftsubsubsecnumwidth}{5.2em}

\setlength{\cftbeforesecskip}{0.5em}
\setlength{\cftbeforesubsecskip}{0.4em}
\setlength{\cftbeforesubsubsecskip}{0.4em}

\begin{document}
\pagenumbering{roman}
\begin{titlepage}
\vspace*{.5in}
\begin{center}
\epsfbox{tm.eps}\\
\vspace*{.5in}
{\huge {\WRspice} Reference Manual}\\
\vspace{.5in}
{\large\sf Whiteley Research Incorporated}\\
  Sunnyvale, CA 94086\\
\vspace{.5in}
{\large Release @RELEASE@\\
@DATE@}\\
\end{center}
\vspace{.5in}
\begin{quote}
\copyright{} Whiteley Research Incorporated, 2025.

% commands, menu entries, and pop-up names: {\cb }
% variables, vectors: {\et }
% example input, operating sys commands and files, functions: {\vt }

{\WRspice} is part of the {\XicTools} software package for
integrated circuit design from Whiteley Research Inc.  {\WRspice}
was authored by S.~R.  Whiteley, with extensive adaptation of the
Berkeley SPICE3 program.  This manual was prepared by Whiteley
Research Inc., acknowledging the material originally authored by the
developers of SPICE3 in the Electrical Engineering and Computer
Sciences Department of the University of California, Berkeley. 

{\WRspice}, and the entire {\XicTools} suite, including this manual,
is provided as open-source under the Apache-2.0 license, as much as
applicable per individual tools, some of which are GNU-licensed.

{\WRspice} and subsidiary programs and utilities are offered as-is,
and the suitability of these programs for any purpose or application
must be established by the user, as neither Whiteley Research Inc.,
or the University of California can imply or guarantee such
suitability.
\end{quote}
\end{titlepage}
This page intentionally left blank.
\newpage
\tableofcontents
\newpage
\ifodd\value{page}\else This page intentionally left blank. \fi
\include{intro}
\ifodd\value{page}\else This page intentionally left blank. \fi
\include{format}
\ifodd\value{page}\else This page intentionally left blank. \fi
\include{useriface}
\ifodd\value{page}\else This page intentionally left blank. \fi
\include{commands}
\ifodd\value{page}\else This page intentionally left blank. \fi
\include{margin}
\ifodd\value{page}\else This page intentionally left blank. \fi
\include{appendix}
\ifodd\value{page}\else This page intentionally left blank. \fi
\include{utilities}
\ifodd\value{page}\else This page intentionally left blank. \fi
\include{bib}
\ifodd\value{page}\else This page intentionally left blank. \fi
\printindex
\newpage
This page intentionally left blank.
\end{document}
