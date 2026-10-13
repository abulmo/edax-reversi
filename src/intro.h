/**
 * @mainpage
 *
 * Edax : a strong Othello program.
 * - copyleft (c) 1998-2025
 * - version: 4.7 (2024-10-?)
 * - author: Richard A Delorme
 * - email: edax-reversi@orange.fr
 * - author: Toshihiko Okuhara
 * - email: okuhara@amy.hi-ho.ne.jp
 *
 *
 * @section lic Licence
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *
 * @section dl Download
 * You can download the source here:
 * - https://github.com/abulmo/edax-reversi
 * - https://github.com/okuhara/edax-reversi-AVX
 * - web site (Japanese): http://www.amy.hi-ho.ne.jp/okuhara/edax.htm

 *
 * @section pres Presentation
 * Edax is a very strong and fast Othello program.
 * For the purpose of strength, I made the following choices:
 *    -# Usage of the C language. This is a language that is simple, fast,
 * portable, etc. I tried to conform to the iso-C99 standard, except where non-
 * standard features were necessary, such as time management or multi-threading
 * capabilities. In such cases, code was made to be easily portable, at least
 * between MS-Windows and Unix. It does compile with most of the C compiler
 * available under Windows, linux  & mac OS X.
 *    -# I used the C language as a very high language. I wrote very modular
 * code. I privileged small functions over long pieces of code. I avoided
 * global variables. Instead I organize data into structures passed to functions
 * as pointers. Structure and function naming was carefully chosen to give an
 * object-oriented-programming feeling. My purpose was to make the program as
 * bug free as possible, and as clear as possible for me to understand it.
 *    -# I used the best algorithms I know:
 *         - fast bitboard-based move generators.
 *         - Principal Variation Search (an enhanced alphabeta) [1].
 *         - Multi-way lock-free hash table [2][3].
 *         - Pattern-based Evaluation function [4].
 *         - MultiProbcut-like selective search [4].
 *         - Parallel search using Young Brother Wait Concept (YBWC) [5].
 *
 * \section comp Compilation
 *     use the makefile
 *
 *
 * \section us Usage
 * Usage: Edax [options] option_file
 *
 * Options:
 * - ?|help                       show this message.
 * - o|option-file                read options from this file.
 * - v|version                    display the version number.
 * - name [string]                set Edax name to [string].
 * - verbose [n]                  verbosity level.
 * - q                            silent mode (eq. -verbose 0).
 * - vv                           very verbose mode (eq. -verbose 2).
 * - noise [n]                    noise level.
 * - width [n]                    line width.
 * - h|hash-table-size [n]        hash table size (in bit size).
 * - n|n-tasks [n]                search in parallel using n tasks.
 * - cpu [n]                      search using cpu [n] (if parallel search is off).
 * - l|level [n]                  search using limited depth.
 * - t|game-time [n]              search using limited time per game.
 * - move-time [n]                search using limited time per move.
 * - ponder [on/off]              search during opponent time.
 * - eval-file                    read eval weight from this file.
 * - book-file                    load opening book from this file.
 * - book-usage [on/off]          play from the opening book.
 * - book-randomness [n]          play various but worse moves from the opening book.
 * - auto-start [on/off]          automatically restart a new game.
 * - auto-swap [on/off]           automatically Edax's color between games
 * - auto-store [on/off]          automatically save played games
 * - game-file [n]                file to store all played game/s.
 * - search-log-file [file]       file to store search detailed output/s.
 * - ui-log-file [file]           file to store input/output to the (U)ser (I)nterface.
 *
 *
 * \section hist History
 * - version 1.0: 1998. Written in C++ for windows, with a graphical interface.
 * Although most of the above algorithms were used, the program was slow and
 * buggy.
 * - version 2.0: 17/02/2000. Written in C for linux with a user interface in
 * console mode. Much more stable and faster. This version won the French
 * Championship for Othello Programming.
 * - version 2.3: 30/06/2000. Enhanced version.
 * - version 3.0: 02/02/2001. Code more modular and better structured. Faster code.
 * - version 3.1: 31/10/2001. Enhanced version.
 * - version 3.2: 28/11/2002. Enhanced version. New Evaluation Function.
 * - version 3.3: 2005-2007. Enhanced version. Code addition started to make the
 * whole program clumsy and buggy.
 * - version 4.0: 04/2010. Bitboard based. Optimized for 64-bits CPU:
 * 2× faster than version 3.3 on such computers. Introduction of parallel search
 * 3.8× faster on a quad core CPU than on a single CPU.
 * - version 4.1: 02/2011. Several bug fixes, cleaner code & small improvements.
 * Pondering (thinking on opponent's time) has been improved. Stable disc
 * evaluation is more accurate. Introduction of Multi-PV search. Automatic
 * detection of the number of cpu. Inclusion of GGS code (need to be improved).
 * Better midgame speed and scalability.
 * - version 4.2: Several bug fixes. Support of Winboard protocol.
 * Better support of nboard protocol (which randomly worked before). Better
 * event support to communicate with GUI.
 * - version 4.2.1: Several bug fixes.
 * - version 4.2.2: Several bug fixes. XBoard protocol should work OK now.
 * - version 4.3: Bug fixes. Cleaner code. Enhancement of GGS protocol support.
 * - version 4.4: Bug fixes. Cleaner code. Enhancement of GGS protocol support.
 *   Faster move generator provided by Toshihiko Okuhara.
 * - version 4.5: 2023 code vectorisation (AVXX2) for x86_64 x86_32 & ARM Neon
 *   by Toshihiko Okuhara giving 7 to 20% faster code.
 * - version 4.6: 10/2024 Port to C17. Bug fixes. Code cleaning. Remove 32 bit support.
 * - version 4.7: Bug fixes. Code cleaning. A few new commands. Enhanced parallel search: ~10% faster multithreaded code.
 *
 * \htmlonly
 * <p> Speed comparison between versions: </p>
 * <table>
 *  <tr> <th> Version </th> <th> fforum-20-39 </th> <th> fforum-40-59 </th> <th> speed </th> </tr>
 *  <tr> <td> 2.3 </td> <td> 0:24.1 </td> <td> 1:34:21.4 </td> <td>  7.3 MN/s </td> </tr>
 *  <tr> <td> 3.0 </td> <td> 0:14.8 </td> <td> 1:37:41.0 </td> <td>  7.5 MN/s </td> </tr>
 *  <tr> <td> 3.3 </td> <td> 0:10.8 </td> <td> 0:51:00.1 </td> <td> 17.3 MN/s </td> </tr>
 *  <tr> <td> 4.0</td> <td> 0:03.35 </td> <td> 0:13:07.9 </td> <td> 32.2 MN/s </td> </tr>
 *  <tr> <td> 4.4</td> <td> 0:03.0 </td> <td> 0:11:08.2 </td> <td> 37.1 MN/s </td> </tr>
 *  <tr> <td> 4.6</td> <td> 0:01.99 </td> <td> 0:07:21.8 </td> <td> 56.9 MN/s </td> </tr>
 *  <tr> <td> 4.7</td> <td> 0:01.91 </td> <td> 0:07:13.8 </td> <td> 57.9 MN/s </td> </tr>
 *  <tr> <th> Version </th> <th> fforum-20-39 </th> <th> fforum-40-59 </th> <th> speed </th> </tr>
 *  <tr> <td> 4.0 × 32</td> <td> 0:00.98 </td> <td> 01:11.46 </td> <td> 399 MN/s </td> </tr>
 *  <tr> <td> 4.4 × 32</td> <td> 0:00.53 </td> <td> 00:45.83 </td> <td> 604 GN/s </td> </tr>
 *  <tr> <td> 4.6 × 32</td> <td> 0:00.53 </td> <td> 00:31.46 </td> <td> 882 GN/s </td> </tr>
 *  <tr> <td> 4.7 × 32</td> <td> 0:00.46 </td> <td> 00:27.26 </td> <td> 1031 GN/s </td> </tr>
 * </table>
 * <p>The test has been done on an AMD Ryzen 9 5950X 16/32 cores CPU running at 4 Ghz with 64GiB of memory,
 * running under Fedora 44.0-x86_64 on 2026 october 10.</p>
 *
 * \endhtmlonly
 *
 * \section ref Reference
 * -# Marsland T.A. (1983) Relative efficiency of alpha-beta implementations.
 *     8th IIJCAI. p 763-766.
 * -# Plaat A., Schaeffer J., Wirn P. & de Bruin A.(1996) Exploiting Graph
 *     Properties of %Game Trees.
 * -# Hyatt R. http://www.cis.uab.edu/hyatt/hashing.html
 * -# Buro M. (1997) Experiments with Multi-ProbCut and a New High-Quality Evaluation Function for Othello.
 * Games in AI Research. p 77-96.
 * -# Feldmann R., Monien B., Mysliwietz P. Vornberger O. (1989) Distributed Game-Tree %Search. ICCA Journal, Vol. 12, No. 2, pp. 65-73.
 *
 */
