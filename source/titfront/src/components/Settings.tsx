/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { PropertySections, PropertyTree } from "~/components/PropertyTree";
import { type Appearance, useSettingsStore } from "~/stores/SettingsStore";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export function Settings() {
  const { appearance, setAppearance } = useSettingsStore();

  return (
    <PropertySections>
      <PropertySections.Section name="Settings">
        <PropertyTree>
          <PropertyTree.Property
            type="enum"
            name="Appearance"
            options={["light", "dark"]}
            value={appearance}
            setValue={(x) => setAppearance(x as Appearance)}
          />
          <PropertyTree.Property
            type="enum"
            name="Appearance 2"
            options={["light", "dark"]}
            value={appearance}
            setValue={(x) => setAppearance(x as Appearance)}
          />
        </PropertyTree>
      </PropertySections.Section>
      <PropertySections.Section name="Other" />
    </PropertySections>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
