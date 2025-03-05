/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { cva } from "class-variance-authority";
import {
  type FC,
  Fragment,
  type ReactElement,
  type ReactNode,
  createContext,
  useCallback,
  useContext,
  useEffect,
  useMemo,
  useState,
} from "react";
import type { IconBaseProps } from "react-icons";
import { FiMinimize as MinimizeIcon } from "react-icons/fi";

import { Resizable } from "~/components/Resizable";
import { useMenuStore } from "~/stores/LayoutStore";
import { assert, type Side, cn, iota } from "~/utils";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

interface Action {
  key: string;
  icon: ReactElement<IconBaseProps>;
  callback: () => void;
}

export interface MenuProviderProps {
  actions: Action[];
  addAction: (action: Action) => void;
  removeAction: (key: string) => void;
}

const MenuContext = createContext<MenuProviderProps | null>(null);

export function useMenu() {
  const context = useContext(MenuContext);
  assert(context !== null, "`useMenu` called outside `Menu`!");
  return context;
}

export function useMenuAction(action: Action) {
  const { addAction, removeAction } = useMenu();
  useEffect(() => {
    addAction(action);
    return () => removeAction(action.key);
  }, [action, addAction, removeAction]);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

interface MenuProps {
  side: Side;
  children: ReactElement<MenuItemProps>[];
}

const menuCva = cva("flex text-gray-300", {
  variants: {
    side: {
      left: "flex-row",
      right: "flex-row-reverse",
      top: "flex-col",
      bottom: "flex-col-reverse",
    },
  },
});
const menuBarCva = cva(
  [
    "flex items-center justify-between",
    "bg-gradient-to-bl from-gray-800 to-gray-950",
  ],
  {
    variants: {
      horizontal: {
        true: "flex-col w-16 h-full gap-6",
        false: "w-full h-8 pl-1",
      },
    },
  }
);
const menuItemCva = cva("flex items-center", {
  variants: {
    horizontal: {
      true: "justify-center size-11 my-6 rounded-lg",
      false: "w-30 h-6 pl-1 rounded-sm",
    },
    active: {
      true: [
        "inset-shadow-sm inset-shadow-gray-700",
        "bg-gradient-to-bl from-gray-700 to-indigo-900 text-gray-100",
      ],
      false: [
        "hover:bg-gradient-to-bl hover:from-gray-700 hover:to-gray-800",
        "hover:text-gray-100 hover:shadow-xl",
      ],
    },
  },
});

export const Menu: FC<MenuProps> & { Item: FC<MenuItemProps> } = ({
  side,
  children = [],
}: MenuProps) => {
  const { size, setSize, activeItem, setActiveItem } = useMenuStore(side);
  const [customActions, setCustomActions] = useState<Action[]>([]);

  // Actions setup.
  const addAction = useCallback((action: Action) => {
    setCustomActions((prev) => [...prev, action]);
  }, []);
  const removeAction = useCallback((key: string) => {
    setCustomActions((prev) => prev.filter((action) => action.key !== key));
  }, []);
  const actions = useMemo<MenuProviderProps>(
    () => ({
      actions: [
        ...customActions,
        {
          key: "hide",
          icon: <MinimizeIcon size={16} />,
          callback: () => setActiveItem(-1),
        },
      ],
      addAction,
      removeAction,
    }),
    [customActions, addAction, removeAction, setActiveItem]
  );

  const maxGroup = children.reduce(
    (max, child) => Math.max(max, child.props.group),
    0
  );
  const horizontal = side === "left" || side === "right";
  return (
    <div className={menuCva({ side })}>
      {/* Menu bar. */}
      <div className={menuBarCva({ horizontal })}>
        {iota(maxGroup + 1).map((group) => (
          <div key={group} className={cn(horizontal || "flex items-center")}>
            {children.map(
              (item, index) =>
                item.props.group === group && (
                  // biome-ignore lint/suspicious/noArrayIndexKey: <explanation>
                  <Fragment key={index}>
                    <button
                      type="button"
                      className={menuItemCva({
                        horizontal,
                        active: index === activeItem,
                      })}
                      onClick={() => setActiveItem(index)}
                    >
                      {item.props.icon}
                      {horizontal || (
                        <span className="ml-1">{item.props.name}</span>
                      )}
                    </button>
                    {horizontal || (
                      <div className="w-[1px] h-6 mx-1 bg-gray-600" />
                    )}
                  </Fragment>
                )
            )}
          </div>
        ))}
      </div>
      {/* Active item. */}
      {activeItem !== -1 && (
        <Resizable side={side} size={size} setSize={setSize}>
          <MenuContext.Provider value={actions}>
            {children[activeItem]}
          </MenuContext.Provider>
        </Resizable>
      )}
    </div>
  );
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

interface MenuItemProps {
  name: string;
  icon: ReactElement<IconBaseProps>;
  group: number;
  children?: ReactNode;
}

const MenuItem: FC<MenuItemProps> = ({ name, children }) => {
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
          {actions.map(({ key, icon, callback }) => (
            <button
              type="button"
              key={key}
              className={cn(
                "w-6 h-6 mr-2 rounded flex items-center justify-center",
                "hover:bg-gray-500"
              )}
              onClick={callback}
            >
              {icon}
            </button>
          ))}
        </div>
      </div>
      {/* Contents. */}
      <div
        className={cn("flex-grow m-1 rounded-lg overflow-auto", "bg-gray-900")}
      >
        {children}
      </div>
    </div>
  );
};

Menu.Item = MenuItem;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
