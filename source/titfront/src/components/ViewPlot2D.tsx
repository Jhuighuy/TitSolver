/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import { Box } from "@radix-ui/themes";
import {
  CartesianGrid,
  Legend,
  Line,
  LineChart,
  ResponsiveContainer,
  Tooltip,
  XAxis,
  YAxis,
} from "recharts";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const data = [
  { name: "0.1", p: 0 },
  { name: "0.2", p: 0 },
  { name: "0.3", p: 0 },
  { name: "0.4", p: 0 },
  { name: "0.5", p: 0 },
  { name: "0.6", p: 0 },
  { name: "0.7", p: 0 },
  { name: "0.8", p: 0 },
  { name: "0.9", p: 0 },
  { name: "1", p: 0 },
  { name: "1.1", p: 0 },
  { name: "1.2", p: 0 },
  { name: "1.3", p: 0 },
  { name: "1.4", p: 0 },
  { name: "1.5", p: 0 },
  { name: "1.6", p: 0 },
  { name: "1.7", p: 0 },
  { name: "1.8", p: 0 },
  { name: "1.9", p: 0 },
  { name: "2", p: 0 },
  { name: "2.1", p: 0 },
  { name: "2.2", p: 0 },
  { name: "2.3", p: 0 },
  { name: "2.4", p: 0 },
  { name: "2.5", p: 0 },
  { name: "2.6", p: 0.00717695 },
  { name: "2.7", p: 0.103942 },
  { name: "2.8", p: 0.228357 },
  { name: "2.9", p: 0.32966 },
  { name: "3", p: 0.33484 },
  { name: "3.1", p: 0.336442 },
  { name: "3.2", p: 0.35708 },
  { name: "3.3", p: 0.36059 },
  { name: "3.4", p: 0.363747 },
  { name: "3.5", p: 0.365435 },
  { name: "3.6", p: 0.390263 },
  { name: "3.7", p: 0.401401 },
  { name: "3.8", p: 0.404943 },
  { name: "3.9", p: 0.419139 },
  { name: "4", p: 0.443131 },
  { name: "4.1", p: 0.440291 },
  { name: "4.2", p: 0.443581 },
  { name: "4.3", p: 0.449644 },
  { name: "4.4", p: 0.458656 },
  { name: "4.5", p: 0.457238 },
  { name: "4.6", p: 0.459331 },
  { name: "4.7", p: 0.459774 },
  { name: "4.8", p: 0.46639 },
  { name: "4.9", p: 0.462976 },
  { name: "5", p: 0.45468 },
  { name: "5.1", p: 0.456557 },
  { name: "5.2", p: 0.457362 },
  { name: "5.3", p: 0.45283 },
  { name: "5.4", p: 0.458047 },
  { name: "5.5", p: 0.452118 },
  { name: "5.6", p: 0.459724 },
  { name: "5.7", p: 0.467879 },
  { name: "5.8", p: 0.471994 },
  { name: "5.9", p: 0.491091 },
  { name: "6", p: 0.512643 },
  { name: "6.1", p: 0.651472 },
  { name: "6.2", p: 0.724756 },
  { name: "6.3", p: 0.954108 },
  { name: "6.4", p: 0.745786 },
  { name: "6.5", p: 0.525792 },
  { name: "6.6", p: 0.462682 },
  { name: "6.7", p: 0.460261 },
  { name: "6.8", p: 0.57475 },
  { name: "6.9", p: 0.343867 },
];

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export function ViewPlot2D() {
  return (
    <Box
      width="100%"
      height="100%"
      p="6"
      overflow="hidden"
      className="bg-gradient-to-bl from-gray-100 to-gray-200 dark:from-gray-700 dark:to-gray-800"
    >
      <ResponsiveContainer width="100%" height="100%" className="outline-none">
        <LineChart data={data}>
          <CartesianGrid strokeDasharray="3 3" />
          <XAxis dataKey="name" />
          <YAxis />
          <Tooltip />
          <Legend />
          <Line type="monotone" dataKey="p" stroke="#82ca9d" />
        </LineChart>
      </ResponsiveContainer>
    </Box>
  );
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
