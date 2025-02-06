/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import * as THREE from "three";
import { OrbitControls } from "three/examples/jsm/controls/OrbitControls.js";
import {
  FC,
  createContext,
  useContext,
  useEffect,
  useRef,
  useState,
} from "react";

import { Orientation } from "./ViewOrientation";
import { usePython } from "./Python";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export class Viewer {
  readonly canvas: HTMLCanvasElement;
  readonly container: HTMLDivElement;
  readonly renderer: THREE.WebGLRenderer;
  readonly scene: THREE.Scene;
  readonly camera: THREE.PerspectiveCamera;
  readonly controls: OrbitControls;

  constructor(canvas: HTMLCanvasElement) {
    this.canvas = canvas;
    this.container = canvas.parentElement! as HTMLDivElement;
    this.renderer = new THREE.WebGLRenderer({ canvas, antialias: true });
    this.renderer.setClearAlpha(0);
    this.renderer.setAnimationLoop(() => this.animate());

    this.scene = new THREE.Scene();

    // Set up the camera.
    this.camera = new THREE.PerspectiveCamera(
      75,
      this.container.clientWidth / this.container.clientHeight,
      Number.EPSILON,
      1000
    );
    this.camera.position.set(0, 0, 5);
    this.controls = new OrbitControls(this.camera, this.renderer.domElement);
    this.controls.enablePan = true;
    // this.setControlsRotation(Math.PI / 3, Math.PI / 4);
    // TODO: This does not work properly.
    const resizeObserver = new ResizeObserver(() => {
      const width = 0.9 * this.container.clientWidth;
      const height = 0.9 * this.container.clientHeight;
      this.camera.aspect = width / height;
      this.camera.updateProjectionMatrix();
      this.renderer.setSize(width, height, false);
    });
    resizeObserver.observe(this.container, { box: "content-box" });

    // Add a global lights.
    const ambientLight = new THREE.AmbientLight(0xeeeeee);
    this.scene.add(ambientLight);
    const light = new THREE.PointLight(0xffffff);
    light.distance = 1;
    light.position.set(10, 10, -10).normalize();
    this.scene.add(light);

    // this.setupTestData();
  }

  setupTestData() {
    const vertices: number[] = [];
    const values: number[] = [];
    const N_FIXED = 4;
    const H = 0.6;
    const L = 2 * H;
    const dr = H / 80.0;
    const POOL_WIDTH = 5.366 * H;
    const POOL_HEIGHT = 2.5 * H;
    const POOL_M = Math.round(POOL_WIDTH / dr);
    const POOL_N = Math.round(POOL_HEIGHT / dr);
    const WATER_M = Math.round(L / dr);
    const WATER_N = Math.round(H / dr);
    const g = 9.81;
    const rho_0 = 1000;
    for (let i = -N_FIXED; i < POOL_M + N_FIXED; ++i) {
      for (let j = -N_FIXED; j < POOL_N; ++j) {
        const is_fixed = i < 0 || i >= POOL_M || j < 0;
        const is_fluid = i < WATER_M && j < WATER_N;

        if (!is_fixed && !is_fluid) continue;

        vertices.push(dr * (i + 0.5), dr * (j + 0.5), 0);
        values.push(0);
      }
    }
    for (let i = 0; i < vertices.length / 3; i++) {
      const x = vertices[i * 3];
      const y = vertices[i * 3 + 1];
      if (x < 0 || x >= L || y < 0 || y >= H) continue;

      let pressure = rho_0 * g * (H - y);
      for (let n = 1; n < 2; n += 2) {
        const pi = Math.PI;
        pressure -=
          (((8 * rho_0 * g * H) / (pi * pi)) *
            (Math.exp((n * pi * (x - L)) / (2 * H)) *
              Math.cos((n * pi * y) / (2 * H)))) /
          (n * n);
      }
      values[i] = pressure;
    }

    for (let i = 0; i < values.length; i++) {
      vertices[i * 3] -= POOL_WIDTH / 2;
      vertices[i * 3 + 1] -= POOL_HEIGHT / 2;
    }

    this.setupData(vertices, values);
  }

  setupData(vertices: number[], values: number[]) {
    let minValue = Infinity;
    let maxValue = -Infinity;
    for (let i = 0; i < values.length; i++) {
      minValue = Math.min(minValue, values[i]);
      maxValue = Math.max(maxValue, values[i]);
    }
    const range = maxValue - minValue;
    for (let i = 0; i < values.length; i++) {
      values[i] = (values[i] - minValue) / range;
    }

    const geometry = new THREE.BufferGeometry();
    geometry.setAttribute(
      "position",
      new THREE.Float32BufferAttribute(vertices, 3)
    );
    geometry.setAttribute("value", new THREE.Float32BufferAttribute(values, 1));
    const material = new THREE.ShaderMaterial({
      uniforms: {
        pointSize: { value: 0.0075 },
        cameraNear: { value: this.camera.near },
        cameraFar: { value: this.camera.far },
        lightPosition: { value: new THREE.Vector3(10, 10, -10) },
        ambientLightColor: { value: new THREE.Color(0xaaaaaa) },
        pointLightColor: { value: new THREE.Color(0xffffff) },
      },
      vertexShader: `
        uniform float pointSize;
        uniform float cameraNear;
        uniform float cameraFar;
        uniform vec3 lightPosition;
        in float value;
        out float fragValue;
        void main() {
          vec4 mvPosition = modelViewMatrix * vec4(position, 1.0);
          gl_PointSize = pointSize * (cameraFar - cameraNear) / length(mvPosition.xyz);
          gl_Position = projectionMatrix * mvPosition;
          fragValue = value;
        }
      `,
      fragmentShader: `
        in float fragValue;
        uniform vec3 lightPosition;
        uniform vec3 ambientLightColor;
        uniform vec3 pointLightColor;
        vec3 jetColormap(float t) {
          t = clamp(t, 0.0, 1.0);
          float r = smoothstep(0.375, 0.625, t) + smoothstep(0.75, 1.0, t);
          float g = smoothstep(0.0, 0.5, t) - smoothstep(0.75, 1.0, t);
          float b = smoothstep(0.0, 0.25, t) - smoothstep(0.5, 0.75, t);
          return vec3(r, g, b);
        }
        void main() {
          vec2 pos = gl_PointCoord.xy - vec2(0.5);
          vec3 normal = normalize(vec3(pos, sqrt(1.0 - dot(pos, pos))));
          if (length(pos) > 0.5) discard;
          vec3 lightDirection = normalize(lightPosition - vec3(gl_FragCoord));
          float lightIntensity = max(dot(lightDirection, normal), 0.0);
          vec3 color = ambientLightColor + pointLightColor * lightIntensity;
          gl_FragColor = vec4(color * jetColormap(fragValue), 1.0);
        }
      `,
    });
    const particles = new THREE.Points(geometry, material);
    this.scene.add(particles);
  }

  setControlsRotation(polar: number, azimuthal: number) {
    const radius = this.camera.position.length(); // Keep the distance the same
    this.camera.position.setFromSphericalCoords(radius, polar, azimuthal);
    this.controls.update();
  }

  private animate() {
    this.controls.update();
    this.renderer.render(this.scene, this.camera);
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

interface ViewerProviderType {
  viewer: Viewer | null;
  setViewer: (viewer: Viewer | null) => void;
}

const ViewerContext = createContext<ViewerProviderType | null>(null);

export const ViewerProvider: FC<{ children: React.ReactNode }> = ({
  children,
}) => {
  const [viewer, setViewer] = useState<Viewer | null>(null);
  return (
    <ViewerContext.Provider value={{ viewer, setViewer }}>
      {children}
    </ViewerContext.Provider>
  );
};

export function useViewer(): Viewer {
  return useContext(ViewerContext)!.viewer!;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export const ViewerComponent: FC = () => {
  const { viewer, setViewer } = useContext(ViewerContext)!;
  const canvasRef = useRef<HTMLCanvasElement>(null);
  const runCode = usePython();

  useEffect(() => {
    if (canvasRef.current && !viewer) {
      setViewer(new Viewer(canvasRef.current));
    }
    return () => {
      if (viewer) setViewer(null);
    };
  }, [canvasRef, viewer, setViewer]);

  // {'r'   : list(storage.last_series.last_time_step.varyings.find_array('r').data.flatten()),
  //  'rho' : list(storage.last_series.last_time_step.varyings.find_array('rho').data.flatten())}
  useEffect(() => {
    if (viewer) {
      runCode(
        "{'r':list(storage.last_series.last_time_step.varyings.find_array('r').data.flatten())," +
          "'rho' : list(storage.last_series.last_time_step.varyings.find_array('rho').data.flatten())}",
        (result) => {
          const r = (result as { r: number[]; rho: number[] })["r"];
          // Convert N*2 array to N*3 array.
          const rr = [];
          for (let i = 0; i < r.length; i += 2) {
            rr.push(r[i], r[i + 1], 0);
          }
          const rho = (result as { r: number[]; rho: number[] })["rho"];
          viewer.setupData(rr, rho);
        }
      );
    }
  }, [viewer, runCode]);

  return (
    <div className="size-full flex items-center justify-center bg-gradient-to-bl from-gray-700 to-gray-800">
      <Orientation />
      <canvas ref={canvasRef} />
    </div>
  );
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
