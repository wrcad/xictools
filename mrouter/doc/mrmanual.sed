% -----------------------------------------------------------------------------
% MRouter Manual
% Stephen R. Whiteley, Whiteley Research Inc., Sunnyvale CA
% No copyright, open-source
% $Id: mrmanual.sed,v 1.1 2016/08/25 01:26:50 stevew Exp $
% -----------------------------------------------------------------------------

\documentclass[twoside,fleqn]{report}
\makeindex
\usepackage{latexsym,longtable,epsfig,makeidx,tocloft}
\pagestyle{headings}
\setlength{\topmargin}{0in}
\setlength{\textheight}{8.5in}
\setlength{\footskip}{.5in}
\setlength{\oddsidemargin}{.25in}
\setlength{\evensidemargin}{-.25in}
\setlength{\textwidth}{6.25in}
\setlength{\parskip}{1.5ex}
\setcounter{tocdepth}{3}
\newcommand{\rr}{\raggedright}
\newcommand{\rb}[1]{\raisebox{2.0ex}[0pt]{#1}}
\newcommand{\Xic}{\sf\slshape Xic}
\newcommand{\XicII}{\sf\slshape XicII}
\newcommand{\Xiv}{\sf\slshape Xiv}
\newcommand{\XicTools}{\sf\slshape XicTools}
\newcommand{\FileTool}{\sf\slshape FileTool}
\newcommand{\WRspice}{\sf\slshape WRspice}
\newcommand{\MRouter}{\sf\slshape MRouter}
\newcommand{\kb}{\sf\bfseries }  %keyboard keys
\newcommand{\cb}{\bf } %command buttons
\newcommand{\et}{\sf } %emphasized text
\newcommand{\vt}{\tt } %verbatim text
\setlength{\mathindent}{1cm}

% Have to modify the TOC number spacing.  The following lines do this
% without tocloft, but cause trouble in latex2html
%
% \makeatletter
%\renewcommand*\l@section{\@dottedtocline{2}{1.5em}{3.3em}}
%\renewcommand*\l@subsection{\@dottedtocline{3}{4.8em}{4.2em}}
%\renewcommand*\l@subsubsection{\@dottedtocline{4}{9.0em}{4.1em}}
% \makeatother

% \setlength\cftchapindent{0em}
% \setlength\cftchapnumwidth{1.5em}
\setlength\cftsecindent{1.5em}
\setlength\cftsecnumwidth{3.3em}
\setlength\cftsubsecindent{4.8em}
\setlength\cftsubsecnumwidth{4.2em}
\setlength\cftsubsubsecindent{10.5em}

\setlength\cftbeforesecskip{0.5em}
\setlength\cftbeforesubsecskip{0.4em}
\setlength\cftbeforesubsubsecskip{0.4em}

% pulled out of figures translated from xfig, since latex2html
% can't handle this in figure blocks
\gdef\SetFigFont#1#2#3#4#5{%
}
%   \reset@font\fontsize{#1}{#2pt}%
%   \fontfamily{#3}\fontseries{#4}\fontshape{#5}%
%   \selectfont}

% keyboard buttons  {\kb what}
% command buttons   {\cb what}
% !promptline cmds  {\cb !what}
% verbatim text     {\vt what}
% functions         {\vt what()}
% file names        {\vt what}
% env variables     {\et what}
% keywords          {\et what}
% !set variables    {\et what}
% unix cmds         {\et what}
% gnd/vcc           {\et what}

\begin{document}
\pagenumbering{roman}
\begin{titlepage}
\vspace*{.5in}
\begin{center}
\epsfbox{tm.eps}\\
\vspace*{.5in}
{\huge {\MRouter} Reference Manual}\\
\vspace{.75in}
{\large\sf Whiteley Research Incorporated}\\456 Flora Vista Avenue\\
  Sunnyvale, CA 94086\\
\vspace{.5in}
{\large Release @RELEASE@\\
@DATE@}\\
\end{center}
\vspace{.75in}
\begin{quote}
\copyright{} Whiteley Research Incorporated, 2006.

{\MRouter} and subsidiary programs and utilities are offered as-is, and the
suitability of these programs for any purpose or application must be
established by the user as Whiteley Research, Inc. does not imply or
guarantee such suitability.
\end{quote}
\end{titlepage}
This page intentionally left blank.
\newpage
\tableofcontents
\newpage
\ifodd\value{page}\else This page intentionally left blank. \fi
\include{mrman1}
\ifodd\value{page}\else This page intentionally left blank. \fi
\include{mrman2}
\ifodd\value{page}\else This page intentionally left blank. \fi
\include{mrman3}
\ifodd\value{page}\else This page intentionally left blank. \fi
\printindex
\newpage
This page intentionally left blank.
\end{document}
