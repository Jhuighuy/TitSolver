#!/usr/bin/env python3
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# Part of the Tit Solver project, under the MIT License
# See /LICENSE.md for license information.
# SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #≈

from dataclasses import dataclass, field
import xml.etree.ElementTree as ET
from pprint import pprint


@dataclass
class DoxyCompound:
    name: str
    brief_description: str
    detailed_description: str


@dataclass
class DoxyClass:
    name: str
    description: str


# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #


def parse_documentation(xml_file):
    """Parse Doxygen documentation."""
    tree = ET.parse(xml_file)
    root_element = tree.getroot()
    # Process all componds.
    return parse_compounds(root_element)


# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #


def parse_compounds(root_element):
    """Parse all `compounddef` subelements."""
    return filter(
        bool,
        [
            parse_compound(compound_element)
            for compound_element in root_element.findall(".//compounddef")
        ],
    )


def parse_compound(compound_element):
    """Parse a single `compounddef` element."""
    kind = compound_element.attrib["kind"]
    if kind in ("file", "dir", "namespace"):
        return None  # No need to process files and directories.
    elif kind in ("class", "struct"):
        return parse_class(compound_element)
    elif kind in ("concept"):
        return parse_concept(compound_element)
    else:
        raise AttributeError(f"Unkown kind {kind}")


def parse_compound_common(compound_element):
    """Parse a single `compounddef` element's common data."""
    name = compound_element.find("compoundname").text.strip()
    brief_description = parse_text(compound_element.find("briefdescription"))
    detailed_description = parse_text(compound_element.find("detaileddescription"))
    return DoxyCompound(
        name=name,
        brief_description=brief_description,
        detailed_description=detailed_description,
    )


# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #


def parse_class(class_element):
    """Parse a single `compounddef` element that's kind is `class` or `struct`."""
    common = parse_compound_common(class_element)
    return common


def parse_concept(class_element):
    """Parse a single `compounddef` element that's kind is `concept`."""
    common = parse_compound_common(class_element)
    return common


# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #


def parse_text(text_element):
    """Parse a single text element (possibly with formatting)."""
    for x in text_element:
        print(x)
    return str(text_element.text)


# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

if __name__ == "__main__":
    doxygen_xml_file = "./output/cmake_output/src/tit/core/docs.xml"

    pprint(list(parse_documentation(doxygen_xml_file)))
