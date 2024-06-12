/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import React, { useEffect, useState } from "react";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export const App: React.FC = () => {
  const [response, setResponse] = useState("");
  useEffect(() => {
    async function fetchData() {
      const result = await window.electron.fetchFromServer("/hello");
      setResponse(result);
    }
    fetchData();
  }, []);
  return (
    <div>
      <h1 className="text-3xl font-bold underline">Tit Solver</h1>
      <button className="btn btn-primary">Click me</button>
      <h1>Tit Solver</h1>
      <h2>Response from the backend: {response}</h2>
    </div>
  );
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
