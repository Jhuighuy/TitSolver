/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { Box, Flex, IconButton, Separator, Text } from "@radix-ui/themes";
import { type ReactElement, useState } from "react";
import {
  FaMinusSquare as MinusIcon,
  FaPlusSquare as PlusIcon,
} from "react-icons/fa";
import {
  FiChevronDown as ChevronDownIcon,
  FiChevronRight as ChevronRightIcon,
} from "react-icons/fi";

import {
  type ArrayProperty,
  getNestedItems,
  type HandleProperty,
  type Property,
  type RecordProperty,
  type ScalarProperty,
} from "~/components/Property";
import { PropertyEditor } from "~/components/property-editors";
import { cn, iota } from "~/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Property Tree.
//

type PropertyTreePropertyProps =
  | ScalarProperty
  | ((RecordProperty | ArrayProperty) & {
      children?: ReactElement<PropertyTreePropertyProps>[];
    });

// eslint-disable-next-line @typescript-eslint/no-unused-vars
function PropertyTreeProperty(_: PropertyTreePropertyProps) {
  return null;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

type PropertyTreeProps = {
  children?: ReactElement<PropertyTreePropertyProps>[];
};

export function PropertyTree({ children }: Readonly<PropertyTreeProps>) {
  // ---- Folding. -------------------------------------------------------------

  const [unfoldedProperties, setUnfoldedProperties] = useState(
    new Set<string>()
  );

  function isUnfolded(property: FlatProperty & HandleProperty) {
    return unfoldedProperties.has(property.id);
  }

  function unfoldProperty(property: FlatProperty & HandleProperty) {
    setUnfoldedProperties((prevUnfoldedProperties) => {
      const newUnfoldedProperties = new Set(prevUnfoldedProperties);
      newUnfoldedProperties.add(property.id);
      return newUnfoldedProperties;
    });
  }

  function foldProperty(property: FlatProperty & HandleProperty) {
    setUnfoldedProperties((prevUnfoldedProperties) => {
      const newUnfoldedProperties = new Set(prevUnfoldedProperties);
      newUnfoldedProperties.delete(property.id);
      return newUnfoldedProperties;
    });
  }

  // ---- Tree flattening. -----------------------------------------------------

  type FlatProperty = ScalarProperty & {
    depth: number;
    id: string;
  };

  function flattenProperties(
    depth: number,
    parentID: string,
    properties: Property[]
  ): FlatProperty[] {
    const flattened: FlatProperty[] = [];
    for (const property of properties) {
      const id = `${parentID}.${property.name}`;
      if (property.type === "record" || property.type === "array") {
        const handle: HandleProperty & FlatProperty = {
          name: property.name,
          type: "handle",
          target: property,
          depth: depth,
          id: id,
        };
        flattened.push(handle);
        if (isUnfolded(handle)) {
          flattened.push(
            ...flattenProperties(depth + 1, id, getNestedItems(property))
          );
        }
      } else {
        flattened.push({ ...property, depth, id });
      }
    }
    return flattened;
  }

  const properties = children ? children.map((c) => c.props) : [];
  const flatProperties = flattenProperties(0, "", properties);

  // --- Layout. ---------------------------------------------------------------

  return (
    <Flex direction="column">
      {flatProperties.map((property, propertyIndex) => (
        <Flex
          key={property.id}
          px="1"
          className={cn(
            propertyIndex % 2 === 0 && "bg-gray-800",
            "hover:bg-indigo-800"
          )}
        >
          {/* ---- Header. ------------------------------------------------- */}
          <Flex width="50%" direction="row" align="center" gap="1">
            {/* Depth indicator. */}
            {iota(property.depth).map((depthIndex) => (
              <div
                key={depthIndex}
                className="flex items-center justify-center w-2 h-full"
              >
                <div className="w-px h-full bg-gray-600" />
              </div>
            ))}

            {/* Fold icon. */}
            <div className="flex items-center justify-center w-2 h-full">
              {property.type === "handle" &&
                (isUnfolded(property) ? (
                  <IconButton
                    size="1"
                    variant="ghost"
                    onClick={() => foldProperty(property)}
                  >
                    <MinusIcon size={8} />
                  </IconButton>
                ) : (
                  <IconButton
                    size="1"
                    variant="ghost"
                    onClick={() => unfoldProperty(property)}
                  >
                    <PlusIcon size={8} />
                  </IconButton>
                ))}
            </div>

            {/* Name. */}
            <Text size="2">{property.name}</Text>

            {/* Unit. */}
            {property.unit && (
              <Text size="1" color="gray">
                {`[${property.unit}]`}
              </Text>
            )}
          </Flex>

          {/* ---- Editor. ------------------------------------------------- */}
          <Box width="50%" maxWidth="50%" p="1">
            <PropertyEditor property={property} />
          </Box>
        </Flex>
      ))}
    </Flex>
  );
}

PropertyTree.Property = PropertyTreeProperty;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Property Sections.
//

type PropertySectionsSectionProps = {
  name: string;
  children?: ReactElement<PropertyTreeProps>;
};

// eslint-disable-next-line @typescript-eslint/no-unused-vars
function PropertySectionsSection(_: PropertySectionsSectionProps) {
  return null;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

type PropertySectionsProps = {
  children?: ReactElement<PropertySectionsSectionProps>[];
};

export function PropertySections({
  children,
}: Readonly<PropertySectionsProps>) {
  const sections = children?.map((c) => c.props) ?? [];

  const [collapsedSections, setCollapsedSections] = useState<boolean[]>(
    new Array(sections.length).fill(false)
  );

  const toggleSection = (index: number) => {
    setCollapsedSections((prev) =>
      prev.map((collapsed, i) => (i === index ? !collapsed : collapsed))
    );
  };

  return (
    <Flex direction="column" m="2" gap="4">
      {/* ---- Sections. --------------------------------------------------- */}
      {sections.map((section, sectionIndex) => (
        <Flex key={section.name} direction="column">
          {/* ---- Header. ------------------------------------------------- */}
          <Flex direction="row" align="center" gap="2">
            <IconButton
              variant="ghost"
              size="1"
              color="gray"
              onClick={() => toggleSection(sectionIndex)}
            >
              {collapsedSections[sectionIndex] ? (
                <ChevronRightIcon size={16} />
              ) : (
                <ChevronDownIcon size={16} />
              )}
            </IconButton>
            <Text weight="bold" size="2">
              {section.name}
            </Text>
          </Flex>
          <Box pt="2">
            <Separator size="4" color="gray" />
          </Box>

          {/* ---- Properties. --------------------------------------------- */}
          {collapsedSections[sectionIndex] || section.children}
        </Flex>
      ))}
    </Flex>
  );
}

PropertySections.Section = PropertySectionsSection;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
