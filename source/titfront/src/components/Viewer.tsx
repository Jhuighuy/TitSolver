/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import {
  createContext,
  type FC,
  useContext,
  useEffect,
  useMemo,
  useRef,
  useState,
} from "react";
import * as THREE from "three";
import { OrbitControls } from "three/examples/jsm/controls/OrbitControls.js";

import { useServer } from "~/components/Server";
import { Orientation } from "~/components/ViewOrientation";
import { assert } from "~/utils";

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
    const parent = canvas.parentElement;
    assert(parent && parent instanceof HTMLDivElement, "Invalid parent.");
    this.container = parent;
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
    const resizeObserver = new ResizeObserver(() => {
      const width = this.container.clientWidth;
      const height = this.container.clientHeight;
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
  }

  setupData(vertices: number[], values: number[]) {
    let minValue = Number.POSITIVE_INFINITY;
    let maxValue = Number.NEGATIVE_INFINITY;
    for (const value of values) {
      minValue = Math.min(minValue, value);
      maxValue = Math.max(maxValue, value);
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
  const obj = useMemo(() => ({ viewer, setViewer }), [viewer]);
  return (
    <ViewerContext.Provider value={obj}>{children}</ViewerContext.Provider>
  );
};

export function useViewer(): Viewer | null {
  return useContext(ViewerContext)?.viewer ?? null;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export const ViewerComponent: FC = () => {
  const context = useContext(ViewerContext);
  assert(context, "Viewer context is not available.");
  const { viewer, setViewer } = context;
  const canvasRef = useRef<HTMLCanvasElement>(null);
  const runCode = useServer();

  useEffect(() => {
    if (canvasRef.current && !viewer) {
      setViewer(new Viewer(canvasRef.current));
    }
    return () => {
      if (viewer) setViewer(null);
    };
  }, [viewer, setViewer]);

  useEffect(() => {
    if (viewer) {
      runCode("", (result) => {
        const res = result as { r: number[]; rho: number[] };
        const r = res.r;
        const rr = [];
        for (let i = 0; i < r.length; i += 2) rr.push(r[i], r[i + 1], 0);
        const rho = res.rho;
        viewer.setupData(rr, rho);
      });
    }
  }, [viewer, runCode]);

  return (
    <div className="size-full bg-gradient-to-bl from-gray-700 to-gray-800 overflow-hidden">
      <Orientation />
      <canvas ref={canvasRef} className="size-full" />
    </div>
  );
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
