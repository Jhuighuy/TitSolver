/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { useState } from "react";
import { Vector3 } from "three";

import { PropertySections, PropertyTree } from "~/components/PropertyTree";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export function VisualSetup() {
  const [name, setName] = useState("My Camera");
  const [projection, setProjection] = useState("Perspective");
  const [fov, setFOV] = useState(60);
  const [position, setPosition] = useState(new Vector3(0, 0, 0));

  return (
    <PropertySections>
      <PropertySections.Section name="Camera">
        <PropertyTree>
          <PropertyTree.Property
            type="string"
            name="Name"
            value={name}
            setValue={setName}
          />
          <PropertyTree.Property
            type="enum"
            name="Projection"
            value={projection}
            setValue={setProjection}
            options={["Perspective", "Orthographic"]}
          />
          <PropertyTree.Property
            type="float"
            name="Field of view"
            value={fov}
            setValue={setFOV}
            min={10}
            max={120}
          />
          <PropertyTree.Property
            type="record"
            name="Position"
            value={position}
            setValue={setPosition}
          />
        </PropertyTree>
      </PropertySections.Section>
      <PropertySections.Section name="Colors"></PropertySections.Section>
    </PropertySections>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
