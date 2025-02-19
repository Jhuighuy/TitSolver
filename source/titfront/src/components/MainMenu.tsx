/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import {
  FC,
  Fragment,
  ReactNode,
  ReactElement,
  createContext,
  useContext,
} from "react";
import { IconBaseProps } from "react-icons";
import { FiMinimize as MinimizeIcon } from "react-icons/fi";

import {
  HorizontalResizableDiv,
  VerticalResizableDiv,
} from "~/components/ResizableDiv";
import { useMenuStore } from "~/stores/LayoutStore";
import { cn } from "~/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

interface Action {
  key: string;
  icon: ReactElement<IconBaseProps>;
  action: () => void;
}

export interface MenuProviderProps {
  actions: Action[];
}

const MenuContext = createContext<MenuProviderProps | null>(null);

export function useMenu() {
  return useContext(MenuContext)!;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export interface MenuItemProps {
  name: string;
  icon: ReactElement<IconBaseProps>;
  group: number;
  children?: ReactNode;
}

export const MenuItem: FC<MenuItemProps> = ({ name, children }) => {
  const { actions } = useMenu();
  return (
    <div
      className={cn(
        "size-full flex flex-col",
        "bg-gradient-to-bl from-gray-700 to-gray-800"
      )}
    >
      {/* Title. */}
      <div className="h-4 m-1 mt-2 flex flex-row items-center justify-between">
        <span className="ml-2 font-medium truncate">{name.toUpperCase()}</span>
        <div className="flex flex-row items-center">
          {actions.map(({ key, icon, action }) => (
            <button
              key={key}
              className={cn(
                "w-6 h-6 mr-2 rounded flex items-center justify-center",
                "hover:bg-gray-500"
              )}
              onClick={action}
            >
              {icon}
            </button>
          ))}
        </div>
      </div>
      {/* Contents. */}
      <div
        className={cn(
          "flex-grow m-1 rounded-lg overflow-auto",
          "bg-gradient-to-br from-gray-800 to-gray-900"
        )}
      >
        {children}
      </div>
    </div>
  );
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export interface MenuProps {
  children: ReactElement<MenuItemProps>[];
}

export const LeftMenu: FC<MenuProps> = ({ children }) => {
  const {
    size: width,
    setSize: setWidth,
    activeItem,
    setActiveItem,
  } = useMenuStore("left");

  const actions: Action[] = [
    {
      key: "hide",
      icon: <MinimizeIcon size={16} />,
      action: () => setActiveItem(-1),
    },
  ];

  const maxGroup = children.reduce(
    (max, child) => Math.max(max, child.props.group),
    0
  );

  const baseItemCn = cn(
    "size-11 mt-6 flex flex-col items-center justify-center rounded-lg"
  );
  const itemCn = cn(
    baseItemCn,
    "hover:bg-gradient-to-bl hover:from-gray-700 hover:to-gray-800",
    "hover:text-gray-100 hover:shadow-xl"
  );
  const activeItemCn = cn(
    baseItemCn,
    "inset-shadow-sm inset-shadow-gray-700",
    "bg-gradient-to-bl from-gray-700 to-indigo-900 text-gray-100"
  );
  return (
    <div className="flex flex-row bg-black text-gray-300">
      {/* Menu items. */}
      <div
        className={cn(
          "min-w-16 max-w-16 h-full pb-6",
          "flex flex-col items-center justify-between",
          "bg-gradient-to-bl from-gray-800 to-gray-950"
        )}
      >
        {[...Array(maxGroup + 1).keys()].map((group) => (
          <div key={group}>
            {children.map(
              (item, index) =>
                item.props.group == group && (
                  <button
                    key={index}
                    className={index === activeItem ? activeItemCn : itemCn}
                    onClick={() => setActiveItem(index)}
                  >
                    {item.props.icon}
                  </button>
                )
            )}
          </div>
        ))}
      </div>
      {/* Active item. */}
      {activeItem !== -1 && (
        <HorizontalResizableDiv width={width} setWidth={setWidth}>
          <MenuContext.Provider value={{ actions }}>
            {children[activeItem]}
          </MenuContext.Provider>
        </HorizontalResizableDiv>
      )}
    </div>
  );
};

export const BottomMenu: FC<MenuProps> = ({ children }) => {
  const {
    size: height,
    setSize: setHeight,
    activeItem,
    setActiveItem,
  } = useMenuStore("bottom");

  const actions: Action[] = [
    {
      key: "hide",
      icon: <MinimizeIcon size={16} />,
      action: () => setActiveItem(-1),
    },
  ];

  const maxGroup = children.reduce(
    (max, child) => Math.max(max, child.props.group),
    0
  );

  const baseItemCn = cn(
    "w-30 h-6 pl-1 flex flex-row items-center rounded-sm",
    "text-gray-300 fond-semibold"
  );
  const itemCn = cn(
    baseItemCn,
    "hover:bg-gradient-to-b hover:from-gray-700 hover:to-gray-800",
    "hover:text-gray-100 hover:shadow-xl"
  );
  const activeItemCn = cn(
    baseItemCn,
    "inset-shadow-sm inset-shadow-gray-700",
    "bg-gradient-to-bl from-gray-700 to-indigo-900 text-gray-100"
  );
  return (
    <div className="flex flex-col bg-black text-gray-300">
      {/* Active item. */}
      {activeItem !== -1 && (
        <VerticalResizableDiv height={height} setHeight={setHeight}>
          <MenuContext.Provider value={{ actions }}>
            {children[activeItem]}
          </MenuContext.Provider>
        </VerticalResizableDiv>
      )}
      {/* Menu items. */}
      <div
        className={cn(
          "w-full h-8 pl-1 flex flex-row items-center",
          "bg-gradient-to-bl from-gray-800 to-gray-950"
        )}
      >
        {[...Array(maxGroup + 1).keys()].map((group) => (
          <div key={group} className="flex flex-row items-center">
            {children.map(
              (item, index) =>
                item.props.group == group && (
                  <Fragment key={index}>
                    <button
                      className={index === activeItem ? activeItemCn : itemCn}
                      onClick={() => setActiveItem(index)}
                    >
                      {item.props.icon}
                      <span className="ml-2 mr-1">{item.props.name}</span>
                    </button>
                    <div className="w-[1px] h-6 mr-1 ml-1 bg-gray-500" />
                  </Fragment>
                )
            )}
          </div>
        ))}
      </div>
    </div>
  );
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
