
/*=======================================================================*
 *                                                                       *
 *  XICTOOLS Integrated Circuit Design System                            *
 *  Copyright (c) 2005 Whiteley Research Inc, all rights reserved.       *
 *                                                                       *
 * MOZY html viewer application files                                    *
 *                                                                       *
 * Based on previous work identified below.                              *
 *-----------------------------------------------------------------------*
 * This file is part of the gtkhtm widget library.  The gtkhtm library
 * was derived from the gtk-xmhtml library by:
 *
 *   Stephen R. Whiteley  <srw@wrcad.com>
 *   Whiteley Research Inc.
 *-----------------------------------------------------------------------*
 *  The gtk-xmhtml widget was derived from the XmHTML library by
 *  Miguel de Icaza  <miguel@nuclecu.unam.mx> and others from the GNOME
 *  project.
 *  11/97 - 2/98
 *-----------------------------------------------------------------------*
 * Author:  newt
 * (C)Copyright 1995-1996 Ripley Software Development
 * All Rights Reserved
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *-----------------------------------------------------------------------*
 * $Id: htm_escape.h,v 1.4 2013/06/17 16:10:28 stevew Exp $
 *-----------------------------------------------------------------------*/

#ifndef ESCAPES_H
#define ESCAPES_H


#define USE_UTF8

namespace {
#ifdef USE_UTF8

struct esc_t
{
    esc_t(const char *a, const char *s, const char *c)
        {
            name = a;
            esc = s;
            enc = c;
        }

    const char *name;
    const char *esc;
    const char *enc;
};

esc_t encodings[] = {
esc_t("nbsp",      "#160",  "\xC2\xA0"), // no-break space = non-breaking space     U+00A0  ISOnum 
esc_t("iexcl",     "#161",  "\xC2\xA1"), // inverted exclamation mark       U+00A1  ISOnum 
esc_t("cent",      "#162",  "\xC2\xA2"), // cent sign       U+00A2  ISOnum 
esc_t("pound",     "#163",  "\xC2\xA3"), // pound sign      U+00A3  ISOnum 
esc_t("curren",    "#164",  "\xC2\xA4"), // currency sign   U+00A4  ISOnum 
esc_t("yen",       "#165",  "\xC2\xA5"), // yen sign = yuan sign    U+00A5  ISOnum 
esc_t("brvbar",    "#166",  "\xC2\xA6"), // broken bar = broken vertical bar        U+00A6  ISOnum 
esc_t("sect",      "#167",  "\xC2\xA7"), // section sign    U+00A7  ISOnum 
esc_t("uml",       "#168",  "\xC2\xA8"), // diaeresis = spacing diaeresis   U+00A8  ISOdia 
esc_t("copy",      "#169",  "\xC2\xA9"), // copyright sign  U+00A9  ISOnum 
esc_t("ordf",      "#170",  "\xC2\xAA"), // feminine ordinal indicator      U+00AA  ISOnum 
esc_t("laquo",     "#171",  "\xC2\xAB"), // left-pointing double angle quotation mark = left pointing guillemet     U+00AB  ISOnum 
esc_t("not",       "#172",  "\xC2\xAC"), // not sign = angled dash  U+00AC  ISOnum 
esc_t("shy",       "#173",  "\xC2\xAD"), // soft hyphen = discretionary hyphen      U+00AD  ISOnum 
esc_t("reg",       "#174",  "\xC2\xAE"), // registered sign = registered trade mark sign    U+00AE  ISOnum 
esc_t("macr",      "#175",  "\xC2\xAF"), // macron = spacing macron = overline = APL overbar        U+00AF  ISOdia 
esc_t("deg",       "#176",  "\xC2\xB0"), // degree sign     U+00B0  ISOnum 
esc_t("plusmn",    "#177",  "\xC2\xB1"), // plus-minus sign = plus-or-minus sign    U+00B1  ISOnum 
esc_t("sup2",      "#178",  "\xC2\xB2"), // superscript two = superscript digit two = squared       U+00B2  ISOnum 
esc_t("sup3",      "#179",  "\xC2\xB3"), // superscript three = superscript digit three = cubed     U+00B3  ISOnum 
esc_t("acute",     "#180",  "\xC2\xB4"), // acute accent = spacing acute    U+00B4  ISOdia
esc_t("micro",     "#181",  "\xC2\xB5"), // micro sign      U+00B5  ISOnum
esc_t("para",      "#182",  "\xC2\xB6"), // pilcrow sign = paragraph sign   U+00B6  ISOnum
esc_t("middot",    "#183",  "\xC2\xB7"), // middle dot = Georgian comma = Greek middle dot  U+00B7  ISOnum
esc_t("cedil",     "#184",  "\xC2\xB8"), // cedilla = spacing cedilla       U+00B8  ISOdia
esc_t("sup1",      "#185",  "\xC2\xB9"), // superscript one = superscript digit one U+00B9  ISOnum
esc_t("ordm",      "#186",  "\xC2\xBA"), // masculine ordinal indicator     U+00BA  ISOnum
esc_t("raquo",     "#187",  "\xC2\xBB"), // right-pointing double angle quotation mark = right pointing guillemet   U+00BB  ISOnum
esc_t("frac14",    "#188",  "\xC2\xBC"), // vulgar fraction one quarter = fraction one quarter      U+00BC  ISOnum
esc_t("frac12",    "#189",  "\xC2\xBD"), // vulgar fraction one half = fraction one half    U+00BD  ISOnum
esc_t("frac34",    "#190",  "\xC2\xBE"), // vulgar fraction three quarters = fraction three quarters        U+00BE  ISOnum
esc_t("iquest",    "#191",  "\xC2\xBF"), // inverted question mark = turned question mark   U+00BF  ISOnum
esc_t("Agrave",    "#192",  "\xC3\x80"), // latin capital letter A with grave = latin capital letter A grave        U+00C0  ISOlat1
esc_t("Aacute",    "#193",  "\xC3\x81"), // latin capital letter A with acute       U+00C1  ISOlat1
esc_t("Acirc",     "#194",  "\xC3\x82"), // latin capital letter A with circumflex  U+00C2  ISOlat1
esc_t("Atilde",    "#195",  "\xC3\x83"), // latin capital letter A with tilde       U+00C3  ISOlat1
esc_t("Auml",      "#196",  "\xC3\x84"), // latin capital letter A with diaeresis   U+00C4  ISOlat1
esc_t("Aring",     "#197",  "\xC3\x85"), // latin capital letter A with ring above = latin capital letter A ring    U+00C5  ISOlat1
esc_t("AElig",     "#198",  "\xC3\x86"), // latin capital letter AE = latin capital ligature AE     U+00C6  ISOlat1
esc_t("Ccedil",    "#199",  "\xC3\x87"), // latin capital letter C with cedilla     U+00C7  ISOlat1
esc_t("Egrave",    "#200",  "\xC3\x88"), // latin capital letter E with grave       U+00C8  ISOlat1
esc_t("Eacute",    "#201",  "\xC3\x89"), // latin capital letter E with acute       U+00C9  ISOlat1
esc_t("Ecirc",     "#202",  "\xC3\x8A"), // latin capital letter E with circumflex  U+00CA  ISOlat1
esc_t("Euml",      "#203",  "\xC3\x8B"), // latin capital letter E with diaeresis   U+00CB  ISOlat1
esc_t("Igrave",    "#204",  "\xC3\x8C"), // latin capital letter I with grave       U+00CC  ISOlat1
esc_t("Iacute",    "#205",  "\xC3\x8D"), // latin capital letter I with acute       U+00CD  ISOlat1
esc_t("Icirc",     "#206",  "\xC3\x8E"), // latin capital letter I with circumflex  U+00CE  ISOlat1
esc_t("Iuml",      "#207",  "\xC3\x8F"), // latin capital letter I with diaeresis   U+00CF  ISOlat1
esc_t("ETH",       "#208",  "\xC3\x90"), // latin capital letter ETH        U+00D0  ISOlat1
esc_t("Ntilde",    "#209",  "\xC3\x91"), // latin capital letter N with tilde       U+00D1  ISOlat1
esc_t("Ograve",    "#210",  "\xC3\x92"), // latin capital letter O with grave       U+00D2  ISOlat1
esc_t("Oacute",    "#211",  "\xC3\x93"), // latin capital letter O with acute       U+00D3  ISOlat1
esc_t("Ocirc",     "#212",  "\xC3\x94"), // latin capital letter O with circumflex  U+00D4  ISOlat1
esc_t("Otilde",    "#213",  "\xC3\x95"), // latin capital letter O with tilde       U+00D5  ISOlat1
esc_t("Ouml",      "#214",  "\xC3\x96"), // latin capital letter O with diaeresis   U+00D6  ISOlat1
esc_t("times",     "#215",  "\xC3\x97"), // multiplication sign     U+00D7  ISOnum
esc_t("Oslash",    "#216",  "\xC3\x98"), // latin capital letter O with stroke = latin capital letter O slash       U+00D8  ISOlat1
esc_t("Ugrave",    "#217",  "\xC3\x99"), // latin capital letter U with grave       U+00D9  ISOlat1
esc_t("Uacute",    "#218",  "\xC3\x9A"), // latin capital letter U with acute       U+00DA  ISOlat1
esc_t("Ucirc",     "#219",  "\xC3\x9B"), // latin capital letter U with circumflex  U+00DB  ISOlat1
esc_t("Uuml",      "#220",  "\xC3\x9C"), // latin capital letter U with diaeresis   U+00DC  ISOlat1
esc_t("Yacute",    "#221",  "\xC3\x9D"), // latin capital letter Y with acute       U+00DD  ISOlat1
esc_t("THORN",     "#222",  "\xC3\x9E"), // latin capital letter THORN      U+00DE  ISOlat1
esc_t("szlig",     "#223",  "\xC3\x9F"), // latin small letter sharp s = ess-zed    U+00DF  ISOlat1
esc_t("agrave",    "#224",  "\xC3\xA0"), // latin small letter a with grave = latin small letter a grave    U+00E0  ISOlat1
esc_t("aacute",    "#225",  "\xC3\xA1"), // latin small letter a with acute U+00E1  ISOlat1
esc_t("acirc",     "#226",  "\xC3\xA2"), // latin small letter a with circumflex    U+00E2  ISOlat1
esc_t("atilde",    "#227",  "\xC3\xA3"), // latin small letter a with tilde U+00E3  ISOlat1
esc_t("auml",      "#228",  "\xC3\xA4"), // latin small letter a with diaeresis     U+00E4  ISOlat1
esc_t("aring",     "#229",  "\xC3\xA5"), // latin small letter a with ring above = latin small letter a ring        U+00E5  ISOlat1
esc_t("aelig",     "#230",  "\xC3\xA6"), // latin small letter ae = latin small ligature ae U+00E6  ISOlat1
esc_t("ccedil",    "#231",  "\xC3\xA7"), // latin small letter c with cedilla       U+00E7  ISOlat1
esc_t("egrave",    "#232",  "\xC3\xA8"), // latin small letter e with grave U+00E8  ISOlat1
esc_t("eacute",    "#233",  "\xC3\xA9"), // latin small letter e with acute U+00E9  ISOlat1
esc_t("ecirc",     "#234",  "\xC3\xAA"), // latin small letter e with circumflex    U+00EA  ISOlat1
esc_t("euml",      "#235",  "\xC3\xAB"), // latin small letter e with diaeresis     U+00EB  ISOlat1
esc_t("igrave",    "#236",  "\xC3\xAC"), // latin small letter i with grave U+00EC  ISOlat1
esc_t("iacute",    "#237",  "\xC3\xAD"), // latin small letter i with acute U+00ED  ISOlat1
esc_t("icirc",     "#238",  "\xC3\xAE"), // latin small letter i with circumflex    U+00EE  ISOlat1
esc_t("iuml",      "#239",  "\xC3\xAF"), // latin small letter i with diaeresis     U+00EF  ISOlat1
esc_t("eth",       "#240",  "\xC3\xB0"), // latin small letter eth  U+00F0  ISOlat1
esc_t("ntilde",    "#241",  "\xC3\xB1"), // latin small letter n with tilde U+00F1  ISOlat1
esc_t("ograve",    "#242",  "\xC3\xB2"), // latin small letter o with grave U+00F2  ISOlat1
esc_t("oacute",    "#243",  "\xC3\xB3"), // latin small letter o with acute U+00F3  ISOlat1
esc_t("ocirc",     "#244",  "\xC3\xB4"), // latin small letter o with circumflex    U+00F4  ISOlat1
esc_t("otilde",    "#245",  "\xC3\xB5"), // latin small letter o with tilde U+00F5  ISOlat1
esc_t("ouml",      "#246",  "\xC3\xB6"), // latin small letter o with diaeresis     U+00F6  ISOlat1
esc_t("divide",    "#247",  "\xC3\xB7"), // division sign   U+00F7  ISOnum
esc_t("oslash",    "#248",  "\xC3\xB8"), // latin small letter o with stroke, = latin small letter o slash  U+00F8  ISOlat1
esc_t("ugrave",    "#249",  "\xC3\xB9"), // latin small letter u with grave U+00F9  ISOlat1
esc_t("uacute",    "#250",  "\xC3\xBA"), // latin small letter u with acute U+00FA  ISOlat1
esc_t("ucirc",     "#251",  "\xC3\xBB"), // latin small letter u with circumflex    U+00FB  ISOlat1
esc_t("uuml",      "#252",  "\xC3\xBC"), // latin small letter u with diaeresis     U+00FC  ISOlat1
esc_t("yacute",    "#253",  "\xC3\xBD"), // latin small letter y with acute U+00FD  ISOlat1
esc_t("thorn",     "#254",  "\xC3\xBE"), // latin small letter thorn        U+00FE  ISOlat1
esc_t("yuml",      "#255",  "\xC3\xBF"), // latin small letter y with diaeresis     U+00FF  ISOlat1
esc_t("quot",      "#34",   "\""), // quotation mark  U+0022  ISOnum
esc_t("amp",       "#38",   "&"), // ampersand       U+0026  ISOnum
esc_t("lt",        "#60",   "<"), // less-than sign  U+003C  ISOnum
esc_t("gt",        "#62",   ">"), // greater-than sign       U+003E  ISOnum
esc_t("apos",      "#39",   "\'"), // apostrophe = APL quote  U+0027  ISOnum
esc_t("OElig",     "#338",  "\xC5\x92"), // latin capital ligature OE       U+0152  ISOlat2
esc_t("oelig",     "#339",  "\xC5\x93"), // latin small ligature oe U+0153  ISOlat2
esc_t("Scaron",    "#352",  "\xC5\xA0"), // latin capital letter S with caron       U+0160  ISOlat2
esc_t("scaron",    "#353",  "\xC5\xA1"), // latin small letter s with caron U+0161  ISOlat2
esc_t("Yuml",      "#376",  "\xC5\xB8"), // latin capital letter Y with diaeresis   U+0178  ISOlat2
esc_t("circ",      "#710",  "\xCB\x86"), // modifier letter circumflex accent       U+02C6  ISOpub
esc_t("tilde",     "#732",  "\xCB\x9C"), // small tilde     U+02DC  ISOdia
esc_t("ensp",      "#8194", "\xE2\x80\x82"), // en space        U+2002  ISOpub
esc_t("emsp",      "#8195", "\xE2\x80\x83"), // em space        U+2003  ISOpub
esc_t("thinsp",    "#8201", "\xE2\x80\x89"), // thin space      U+2009  ISOpub
esc_t("zwnj",      "#8204", "\xE2\x80\x8C"), // zero width non-joiner   U+200C  NEW RFC 2070
esc_t("zwj",       "#8205", "\xE2\x80\x8D"), // zero width joiner       U+200D  NEW RFC 2070
esc_t("lrm",       "#8206", "\xE2\x80\x8E"), // left-to-right mark      U+200E  NEW RFC 2070
esc_t("rlm",       "#8207", "\xE2\x80\x8F"), // right-to-left mark      U+200F  NEW RFC 2070
esc_t("ndash",     "#8211", "\xE2\x80\x93"), // en dash U+2013  ISOpub
esc_t("mdash",     "#8212", "\xE2\x80\x94"), // em dash U+2014  ISOpub
esc_t("lsquo",     "#8216", "\xE2\x80\x98"), // left single quotation mark      U+2018  ISOnum
esc_t("rsquo",     "#8217", "\xE2\x80\x99"), // right single quotation mark     U+2019  ISOnum
esc_t("sbquo",     "#8218", "\xE2\x80\x9A"), // single low-9 quotation mark     U+201A  NEW
esc_t("ldquo",     "#8220", "\xE2\x80\x9C"), // left double quotation mark      U+201C  ISOnum
esc_t("rdquo",     "#8221", "\xE2\x80\x9D"), // right double quotation mark     U+201D  ISOnum
esc_t("bdquo",     "#8222", "\xE2\x80\x9E"), // double low-9 quotation mark     U+201E  NEW
esc_t("dagger",    "#8224", "\xE2\x80\xA0"), // dagger  U+2020  ISOpub
esc_t("Dagger",    "#8225", "\xE2\x80\xA1"), // double dagger   U+2021  ISOpub
esc_t("permil",    "#8240", "\xE2\x80\xB0"), // per mille sign  U+2030  ISOtech
esc_t("lsaquo",    "#8249", "\xE2\x80\xB9"), // single left-pointing angle quotation mark       U+2039  ISO proposed
esc_t("rsaquo",    "#8250", "\xE2\x80\xBA"), // single right-pointing angle quotation mark      U+203A  ISO proposed
esc_t("euro",      "#8364", "\xE2\x82\xAC"), // euro sign       U+20AC  NEW
esc_t("fnof",      "#402",  "\xC6\x92"), // latin small letter f with hook = function = florin      U+0192  ISOtech
esc_t("Alpha",     "#913",  "\xCE\x91"), // greek capital letter alpha      U+0391
esc_t("Beta",      "#914",  "\xCE\x92"), // greek capital letter beta       U+0392
esc_t("Gamma",     "#915",  "\xCE\x93"), // greek capital letter gamma      U+0393  ISOgrk3
esc_t("Delta",     "#916",  "\xCE\x94"), // greek capital letter delta      U+0394  ISOgrk3
esc_t("Epsilon",   "#917",  "\xCE\x95"), // greek capital letter epsilon    U+0395
esc_t("Zeta",      "#918",  "\xCE\x96"), // greek capital letter zeta       U+0396
esc_t("Eta",       "#919",  "\xCE\x97"), // greek capital letter eta        U+0397
esc_t("Theta",     "#920",  "\xCE\x98"), // greek capital letter theta      U+0398  ISOgrk3
esc_t("Iota",      "#921",  "\xCE\x99"), // greek capital letter iota       U+0399
esc_t("Kappa",     "#922",  "\xCE\x9A"), // greek capital letter kappa      U+039A
esc_t("Lambda",    "#923",  "\xCE\x9B"), // greek capital letter lamda      U+039B  ISOgrk3
esc_t("Mu",        "#924",  "\xCE\x9C"), // greek capital letter mu U+039C
esc_t("Nu",        "#925",  "\xCE\x9D"), // greek capital letter nu U+039D
esc_t("Xi",        "#926",  "\xCE\x9E"), // greek capital letter xi U+039E  ISOgrk3
esc_t("Omicron",   "#927",  "\xCE\x9F"), // greek capital letter omicron    U+039F
esc_t("Pi",        "#928",  "\xCE\xA0"), // greek capital letter pi U+03A0  ISOgrk3
esc_t("Rho",       "#929",  "\xCE\xA1"), // greek capital letter rho        U+03A1
esc_t("Sigma",     "#931",  "\xCE\xA3"), // greek capital letter sigma      U+03A3  ISOgrk3
esc_t("Tau",       "#932",  "\xCE\xA4"), // greek capital letter tau        U+03A4
esc_t("Upsilon",   "#933",  "\xCE\xA5"), // greek capital letter upsilon    U+03A5  ISOgrk3
esc_t("Phi",       "#934",  "\xCE\xA6"), // greek capital letter phi        U+03A6  ISOgrk3
esc_t("Chi",       "#935",  "\xCE\xA7"), // greek capital letter chi        U+03A7
esc_t("Psi",       "#936",  "\xCE\xA8"), // greek capital letter psi        U+03A8  ISOgrk3
esc_t("Omega",     "#937",  "\xCE\xA9"), // greek capital letter omega      U+03A9  ISOgrk3
esc_t("alpha",     "#945",  "\xCE\xB1"), // greek small letter alpha        U+03B1  ISOgrk3
esc_t("beta",      "#946",  "\xCE\xB2"), // greek small letter beta U+03B2  ISOgrk3
esc_t("gamma",     "#947",  "\xCE\xB3"), // greek small letter gamma        U+03B3  ISOgrk3
esc_t("delta",     "#948",  "\xCE\xB4"), // greek small letter delta        U+03B4  ISOgrk3
esc_t("epsilon",   "#949",  "\xCE\xB5"), // greek small letter epsilon      U+03B5  ISOgrk3
esc_t("zeta",      "#950",  "\xCE\xB6"), // greek small letter zeta U+03B6  ISOgrk3
esc_t("eta",       "#951",  "\xCE\xB7"), // greek small letter eta  U+03B7  ISOgrk3
esc_t("theta",     "#952",  "\xCE\xB8"), // greek small letter theta        U+03B8  ISOgrk3
esc_t("iota",      "#953",  "\xCE\xB9"), // greek small letter iota U+03B9  ISOgrk3
esc_t("kappa",     "#954",  "\xCE\xBA"), // greek small letter kappa        U+03BA  ISOgrk3
esc_t("lambda",    "#955",  "\xCE\xBB"), // greek small letter lamda        U+03BB  ISOgrk3
esc_t("mu",        "#956",  "\xCE\xBC"), // greek small letter mu   U+03BC  ISOgrk3
esc_t("nu",        "#957",  "\xCE\xBD"), // greek small letter nu   U+03BD  ISOgrk3
esc_t("xi",        "#958",  "\xCE\xBE"), // greek small letter xi   U+03BE  ISOgrk3
esc_t("omicron",   "#959",  "\xCE\xBF"), // greek small letter omicron      U+03BF  NEW
esc_t("pi",        "#960",  "\xCF\x80"), // greek small letter pi   U+03C0  ISOgrk3
esc_t("rho",       "#961",  "\xCF\x81"), // greek small letter rho  U+03C1  ISOgrk3
esc_t("sigmaf",    "#962",  "\xCF\x82"), // greek small letter final sigma  U+03C2  ISOgrk3
esc_t("sigma",     "#963",  "\xCF\x83"), // greek small letter sigma        U+03C3  ISOgrk3
esc_t("tau",       "#964",  "\xCF\x84"), // greek small letter tau  U+03C4  ISOgrk3
esc_t("upsilon",   "#965",  "\xCF\x85"), // greek small letter upsilon      U+03C5  ISOgrk3
esc_t("phi",       "#966",  "\xCF\x86"), // greek small letter phi  U+03C6  ISOgrk3
esc_t("chi",       "#967",  "\xCF\x87"), // greek small letter chi  U+03C7  ISOgrk3
esc_t("psi",       "#968",  "\xCF\x88"), // greek small letter psi  U+03C8  ISOgrk3
esc_t("omega",     "#969",  "\xCF\x89"), // greek small letter omega        U+03C9  ISOgrk3
esc_t("thetasym",  "#977",  "\xCF\x91"), // greek theta symbol      U+03D1  NEW
esc_t("upsih",     "#978",  "\xCF\x92"), // greek upsilon with hook symbol  U+03D2  NEW
esc_t("piv",       "#982",  "\xCF\x96"), // greek pi symbol U+03D6  ISOgrk3
esc_t("bull",      "#8226", "\xE2\x80\xA2"), // bullet = black small circle     U+2022  ISOpub
esc_t("hellip",    "#8230", "\xE2\x80\xA6"), // horizontal ellipsis = three dot leader  U+2026  ISOpub
esc_t("prime",     "#8242", "\xE2\x80\xB2"), // prime = minutes = feet  U+2032  ISOtech
esc_t("Prime",     "#8243", "\xE2\x80\xB3"), // double prime = seconds = inches U+2033  ISOtech
esc_t("oline",     "#8254", "\xE2\x80\xBE"), // overline = spacing overscore    U+203E  NEW 
esc_t("frasl",     "#8260", "\xE2\x81\x84"), // fraction slash  U+2044  NEW
esc_t("weierp",    "#8472", "\xE2\x84\x98"), // script capital P = power set = Weierstrass p    U+2118  ISOamso
esc_t("image",     "#8465", "\xE2\x84\x91"), // black-letter capital I = imaginary part U+2111  ISOamso
esc_t("real",      "#8476", "\xE2\x84\x9C"), // black-letter capital R = real part symbol       U+211C  ISOamso
esc_t("trade",     "#8482", "\xE2\x84\xA2"), // trade mark sign U+2122  ISOnum
esc_t("alefsym",   "#8501", "\xE2\x84\xB5"), // alef symbol = first transfinite cardinal        U+2135  NEW
esc_t("larr",      "#8592", "\xE2\x86\x90"), // leftwards arrow U+2190  ISOnum
esc_t("uarr",      "#8593", "\xE2\x86\x91"), // upwards arrow   U+2191  ISOnum
esc_t("rarr",      "#8594", "\xE2\x86\x92"), // rightwards arrow        U+2192  ISOnum
esc_t("darr",      "#8595", "\xE2\x86\x93"), // downwards arrow U+2193  ISOnum
esc_t("harr",      "#8596", "\xE2\x86\x94"), // left right arrow        U+2194  ISOamsa
esc_t("crarr",     "#8629", "\xE2\x86\xB5"), // downwards arrow with corner leftwards = carriage return U+21B5  NEW
esc_t("lArr",      "#8656", "\xE2\x87\x90"), // leftwards double arrow  U+21D0  ISOtech
esc_t("uArr",      "#8657", "\xE2\x87\x91"), // upwards double arrow    U+21D1  ISOamsa
esc_t("rArr",      "#8658", "\xE2\x87\x92"), // rightwards double arrow U+21D2  ISOtech
esc_t("dArr",      "#8659", "\xE2\x87\x93"), // downwards double arrow  U+21D3  ISOamsa
esc_t("hArr",      "#8660", "\xE2\x87\x94"), // left right double arrow U+21D4  ISOamsa
esc_t("forall",    "#8704", "\xE2\x88\x80"), // for all U+2200  ISOtech
esc_t("part",      "#8706", "\xE2\x88\x82"), // partial differential    U+2202  ISOtech
esc_t("exist",     "#8707", "\xE2\x88\x83"), // there exists    U+2203  ISOtech
esc_t("empty",     "#8709", "\xE2\x88\x85"), // empty set = null set    U+2205  ISOamso
esc_t("nabla",     "#8711", "\xE2\x88\x87"), // nabla = backward difference     U+2207  ISOtech
esc_t("isin",      "#8712", "\xE2\x88\x88"), // element of      U+2208  ISOtech
esc_t("notin",     "#8713", "\xE2\x88\x89"), // not an element of       U+2209  ISOtech
esc_t("ni",        "#8715", "\xE2\x88\x8B"), // contains as member      U+220B  ISOtech
esc_t("prod",      "#8719", "\xE2\x88\x8F"), // n-ary product = product sign    U+220F  ISOamsb
esc_t("sum",       "#8721", "\xE2\x88\x91"), // n-ary summation U+2211  ISOamsb
esc_t("minus",     "#8722", "\xE2\x88\x92"), // minus sign      U+2212  ISOtech
esc_t("lowast",    "#8727", "\xE2\x88\x97"), // asterisk operator       U+2217  ISOtech
esc_t("radic",     "#8730", "\xE2\x88\x9A"), // square root = radical sign      U+221A  ISOtech
esc_t("prop",      "#8733", "\xE2\x88\x9D"), // proportional to U+221D  ISOtech
esc_t("infin",     "#8734", "\xE2\x88\x9E"), // infinity        U+221E  ISOtech
esc_t("ang",       "#8736", "\xE2\x88\xA0"), // angle   U+2220  ISOamso
esc_t("and",       "#8743", "\xE2\x88\xA7"), // logical and = wedge     U+2227  ISOtech
esc_t("or",        "#8744", "\xE2\x88\xA8"), // logical or = vee        U+2228  ISOtech
esc_t("cap",       "#8745", "\xE2\x88\xA9"), // intersection = cap      U+2229  ISOtech
esc_t("cup",       "#8746", "\xE2\x88\xAA"), // union = cup     U+222A  ISOtech
esc_t("int",       "#8747", "\xE2\x88\xAB"), // integral        U+222B  ISOtech
esc_t("there4",    "#8756", "\xE2\x88\xB4"), // therefore       U+2234  ISOtech
esc_t("sim",       "#8764", "\xE2\x88\xBC"), // tilde operator = varies with = similar to       U+223C  ISOtech
esc_t("cong",      "#8773", "\xE2\x89\x85"), // approximately equal to  U+2245  ISOtech
esc_t("asymp",     "#8776", "\xE2\x89\x88"), // almost equal to = asymptotic to U+2248  ISOamsr
esc_t("ne",        "#8800", "\xE2\x89\xA0"), // not equal to    U+2260  ISOtech
esc_t("equiv",     "#8801", "\xE2\x89\xA1"), // identical to    U+2261  ISOtech
esc_t("le",        "#8804", "\xE2\x89\xA4"), // less-than or equal to   U+2264  ISOtech
esc_t("ge",        "#8805", "\xE2\x89\xA5"), // greater-than or equal to        U+2265  ISOtech
esc_t("sub",       "#8834", "\xE2\x8A\x82"), // subset of       U+2282  ISOtech
esc_t("sup",       "#8835", "\xE2\x8A\x83"), // superset of     U+2283  ISOtech
esc_t("nsub",      "#8836", "\xE2\x8A\x84"), // not a subset of U+2284  ISOamsn
esc_t("sube",      "#8838", "\xE2\x8A\x86"), // subset of or equal to   U+2286  ISOtech
esc_t("supe",      "#8839", "\xE2\x8A\x87"), // superset of or equal to U+2287  ISOtech
esc_t("oplus",     "#8853", "\xE2\x8A\x95"), // circled plus = direct sum       U+2295  ISOamsb
esc_t("otimes",    "#8855", "\xE2\x8A\x97"), // circled times = vector product  U+2297  ISOamsb
esc_t("perp",      "#8869", "\xE2\x8A\xA5"), // up tack = orthogonal to = perpendicular U+22A5  ISOtech
esc_t("sdot",      "#8901", "\xE2\x8B\x85"), // dot operator    U+22C5  ISOamsb
esc_t("lceil",     "#8968", "\xE2\x8C\x88"), // left ceiling = APL upstile      U+2308  ISOamsc
esc_t("rceil",     "#8969", "\xE2\x8C\x89"), // right ceiling   U+2309  ISOamsc
esc_t("lfloor",    "#8970", "\xE2\x8C\x8A"), // left floor = APL downstile      U+230A  ISOamsc
esc_t("rfloor",    "#8971", "\xE2\x8C\x8B"), // right floor     U+230B  ISOamsc
esc_t("lang",      "#9001", "\xE2\x8C\xA9"), // left-pointing angle bracket = bra       U+2329  ISOtech
esc_t("rang",      "#9002", "\xE2\x8C\xAA"), // right-pointing angle bracket = ket      U+232A  ISOtech
esc_t("loz",       "#9674", "\xE2\x97\x8A"), // lozenge U+25CA  ISOpub
esc_t("spades",    "#9824", "\xE2\x99\xA0"), // black spade suit        U+2660  ISOpub
esc_t("clubs",     "#9827", "\xE2\x99\xA3"), // black club suit = shamrock      U+2663  ISOpub
esc_t("hearts",    "#9829", "\xE2\x99\xA5"), // black heart suit = valentine    U+2665  ISOpub
esc_t("diams",     "#9830", "\xE2\x99\xA6"), // black diamond suit      U+2666
esc_t(0,            0,      0)
};

#else

namespace
{
    struct escape_data
    {
        const char *escape; // escape character sequence
        char token;         // corresponding iso-char
        int len;            // length of escape sequence
    };
}

// List of all possible HTML escape codes.  This list has been
// alphabetically sorted to speed up the search process.  This list
// contains 198 elements.  The last element is the 0 element.  The
// first part of this table contains the hash escapes, the second part
// contains the named entity escapes.

#define NUM_ESCAPES 198
escape_data escapes[NUM_ESCAPES] =
{
    {"#160;",       '\240', 5},
    {"#161;",       '\241', 5},
    {"#162;",       '\242', 5},
    {"#163;",       '\243', 5},
    {"#164;",       '\244', 5},
    {"#165;",       '\245', 5},
    {"#166;",       '\246', 5},
    {"#167;",       '\247', 5},
    {"#168;",       '\250', 5},
    {"#169;",       '\251', 5},
    {"#170;",       '\252', 5},
    {"#171;",       '\253', 5},
    {"#172;",       '\254', 5},
    {"#173;",       '\255', 5},
    {"#174;",       '\256', 5},
    {"#175;",       '\257', 5},
    {"#176;",       '\260', 5},
    {"#177;",       '\261', 5},
    {"#178;",       '\262', 5},
    {"#179;",       '\263', 5},
    {"#180;",       '\264', 5},
    {"#181;",       '\265', 5},
    {"#182;",       '\266', 5},
    {"#183;",       '\267', 5},
    {"#184;",       '\270', 5},
    {"#185;",       '\271', 5},
    {"#186;",       '\272', 5},
    {"#187;",       '\273', 5},
    {"#188;",       '\274', 5},
    {"#189;",       '\275', 5},
    {"#190;",       '\276', 5},
    {"#191;",       '\277', 5},
    {"#192;",       '\300', 5},
    {"#193;",       '\301', 5},
    {"#194;",       '\302', 5},
    {"#195;",       '\303', 5},
    {"#196;",       '\304', 5},
    {"#197;",       '\305', 5},
    {"#198;",       '\306', 5},
    {"#199;",       '\307', 5},
    {"#200;",       '\310', 5},
    {"#201;",       '\311', 5},
    {"#202;",       '\312', 5},
    {"#203;",       '\313', 5},
    {"#204;",       '\314', 5},
    {"#205;",       '\315', 5},
    {"#206;",       '\316', 5},
    {"#207;",       '\317', 5},
    {"#208;",       '\320', 5},
    {"#209;",       '\321', 5},
    {"#210;",       '\322', 5},
    {"#211;",       '\323', 5},
    {"#212;",       '\324', 5},
    {"#213;",       '\325', 5},
    {"#214;",       '\326', 5},
    {"#215;",       '\327', 5},
    {"#216;",       '\330', 5},
    {"#217;",       '\331', 5},
    {"#218;",       '\332', 5},
    {"#219;",       '\333', 5},
    {"#220;",       '\334', 5},
    {"#221;",       '\335', 5},
    {"#222;",       '\336', 5},
    {"#223;",       '\337', 5},
    {"#224;",       '\340', 5},
    {"#225;",       '\341', 5},
    {"#226;",       '\342', 5},
    {"#227;",       '\343', 5},
    {"#228;",       '\344', 5},
    {"#229;",       '\345', 5},
    {"#230;",       '\346', 5},
    {"#231;",       '\347', 5},
    {"#232;",       '\350', 5},
    {"#233;",       '\351', 5},
    {"#234;",       '\352', 5},
    {"#235;",       '\353', 5},
    {"#236;",       '\354', 5},
    {"#237;",       '\355', 5},
    {"#238;",       '\356', 5},
    {"#239;",       '\357', 5},
    {"#240;",       '\360', 5},
    {"#241;",       '\361', 5},
    {"#242;",       '\362', 5},
    {"#243;",       '\363', 5},
    {"#244;",       '\364', 5},
    {"#245;",       '\365', 5},
    {"#246;",       '\366', 5},
    {"#247;",       '\367', 5},
    {"#248;",       '\370', 5},
    {"#249;",       '\371', 5},
    {"#250;",       '\372', 5},
    {"#251;",       '\373', 5},
    {"#252;",       '\374', 5},
    {"#253;",       '\375', 5},
    {"#254;",       '\376', 5},
    {"#255;",       '\377', 5},
    {"AElig;",      '\306', 6},
    {"Aacute;",     '\301', 7},
    {"Acirc;",      '\302', 6},
    {"Agrave;",     '\300', 7},
    {"Aring;",      '\305', 6},
    {"Atilde;",     '\303', 7},
    {"Auml;",       '\304', 5},
    {"Ccedil;",     '\307', 7},
    {"ETH;",        '\320', 4},
    {"Eacute;",     '\311', 7},
    {"Ecirc;",      '\312', 6},
    {"Egrave;",     '\310', 7},
    {"Euml;",       '\313', 5},
    {"Iacute;",     '\315', 7},
    {"Icirc;",      '\316', 6},
    {"Igrave;",     '\314', 7},
    {"Iuml;",       '\317', 5},
    {"Ntilde;",     '\321', 7},
    {"Oacute;",     '\323', 7},
    {"Ocirc;",      '\324', 6},
    {"Ograve;",     '\322', 7},
    {"Oslash;",     '\330', 7},
    {"Otilde;",     '\325', 7},
    {"Ouml;",       '\326', 5},
    {"THORN;",      '\336', 6},
    {"Uacute;",     '\332', 7},
    {"Ucirc;",      '\333', 6},
    {"Ugrave;",     '\331', 7},
    {"Uuml;",       '\334', 5},
    {"Yacute;",     '\335', 7},
    {"aacute;",     '\341', 7},
    {"acirc;",      '\342', 6},
    {"acute;",      '\264', 6},
    {"aelig;",      '\346', 6},
    {"agrave;",     '\340', 7},
    {"amp;",        '&',    4},
    {"aring;",      '\345', 6},
    {"atilde;",     '\343', 7},
    {"auml;",       '\344', 5},
    {"brvbar;",     '\246', 7},
    {"ccedil;",     '\347', 7},
    {"cedil;",      '\270', 6},
    {"cent;",       '\242', 5},
    {"copy;",       '\251', 5},
    {"curren;",     '\244', 7},
    {"deg;",        '\260', 4},
    {"divide;",     '\367', 7},
    {"eacute;",     '\351', 7},
    {"ecirc;",      '\352', 6},
    {"egrave;",     '\350', 7},
    {"eth;",        '\360', 4},
    {"euml;",       '\353', 5},
    {"frac12;",     '\275', 7},
    {"frac14;",     '\274', 7},
    {"frac34;",     '\276', 7},
    {"gt;",         '>',    3},
    {"hibar;",      '\257', 6},
    {"iacute;",     '\355', 7},
    {"icirc;",      '\356', 6},
    {"iexcl;",      '\241', 6},
    {"igrave;",     '\354', 7},
    {"iquest;",     '\277', 7},
    {"iuml;",       '\357', 5},
    {"laquo;",      '\253', 6},
    {"lt;",         '<',    3},
    {"macr;",       '\257', 5},
    {"micro;",      '\265', 6},
    {"middot;",     '\267', 7},
    {"nbsp;",       '\240', 5},
    {"not;",        '\254', 4},
    {"ntilde;",     '\361', 7},
    {"oacute;",     '\363', 7},
    {"ocirc;",      '\364', 6},
    {"ograve;",     '\362', 7},
    {"ordf;",       '\252', 5},
    {"ordm;",       '\272', 5},
    {"oslash;",     '\370', 7},
    {"otilde;",     '\365', 7},
    {"ouml;",       '\366', 5},
    {"para;",       '\266', 5},
    {"plusmn;",     '\261', 7},
    {"pound;",      '\243', 6},
    {"quot;",       '\"',   5},
    {"raquo;",      '\273', 6},
    {"reg;",        '\256', 4},
    {"sect;",       '\247', 5},
    {"shy;",        '\255', 4},
    {"sup1;",       '\271', 5},
    {"sup2;",       '\262', 5},
    {"sup3;",       '\263', 5},
    {"szlig;",      '\337', 6},
    {"thorn;",      '\376', 6},
    {"times;",      '\327', 6},
    {"uacute;",     '\372', 7},
    {"ucirc;",      '\373', 6},
    {"ugrave;",     '\371', 7},
    {"uml;",        '\250', 4},
    {"uuml;",       '\374', 5},
    {"yacute;",     '\375', 7},
    {"yen;",        '\245', 4},
    {"yuml;",       '\377', 5},
    {0,             0,      0}
};

#endif
}

#endif

