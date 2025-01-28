/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import "@testing-library/jest-dom/vitest";
import { render, screen, waitFor } from "@testing-library/react";
import { describe, it, expect } from "vitest";
import { useState } from "react";
import { z } from "zod";

import { PyConnectionProvider, PyError, usePython } from "~/components/Python";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

describe("usePython", () => {
  it("renders message while connecting", () => {
    render(
      <PyConnectionProvider>
        <div />
      </PyConnectionProvider>
    );

    expect(screen.getByText("Connecting...")).toBeInTheDocument();
  });

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  it("runs code and returns primitives", async () => {
    const TestComponent = () => {
      const [result, setResult] = useState("");
      const runCode = usePython();
      runCode("65 / 5", (result) => {
        expect(typeof result).toBe("number");
        setResult((result as number).toString());
      });
      return <div>{result}</div>;
    };

    render(
      <PyConnectionProvider>
        <TestComponent />
      </PyConnectionProvider>
    );

    await waitFor(() => expect(screen.getByText("13")).toBeInTheDocument());
  });

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  it("runs code and returns objects", async () => {
    const object = { a: 1, b: 2 };
    const objectString = JSON.stringify(object);
    const TestComponent = () => {
      const [result, setResult] = useState("");
      const runCode = usePython();
      runCode(objectString, (result) => {
        const schema = z.object({ a: z.number(), b: z.number() });
        setResult(JSON.stringify(schema.parse(result)));
      });
      return <div>{result}</div>;
    };

    render(
      <PyConnectionProvider>
        <TestComponent />
      </PyConnectionProvider>
    );

    await waitFor(() => {
      expect(screen.getByText(objectString)).toBeInTheDocument();
    });
  });

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  it("sends result to right components", async () => {
    const TestComponent = ({ denum }: { denum: number }) => {
      const [result, setResult] = useState("");
      const runCode = usePython();
      runCode(`65 / ${denum}`, (result) => {
        expect(typeof result).toBe("number");
        setResult((result as number).toString());
      });
      return <div>{`65 / ${denum} = ${result}`}</div>;
    };

    render(
      <PyConnectionProvider>
        <TestComponent denum={5} />
        <TestComponent denum={13} />
      </PyConnectionProvider>
    );

    await waitFor(() => {
      expect(screen.getByText("65 / 5 = 13")).toBeInTheDocument();
      expect(screen.getByText("65 / 13 = 5")).toBeInTheDocument();
    });
  });

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  it("runs code and returns errors", async () => {
    const TestComponent = () => {
      const [result, setResult] = useState("");
      const runCode = usePython();
      runCode("1 / 0", (result) => {
        expect(result).toBeInstanceOf(PyError);
        setResult((result as PyError).toString());
      });
      return <div>{result}</div>;
    };

    render(
      <PyConnectionProvider>
        <TestComponent />
      </PyConnectionProvider>
    );

    await waitFor(() => {
      expect(
        screen.getByText("ZeroDivisionError: division by zero")
      ).toBeInTheDocument();
    });
  });
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
