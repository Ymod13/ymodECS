interface RmlEvent {
    type: string;
    target: RmlElement | null;
    params: Record<string, number | string | boolean>;
}

interface RmlElement {
    getAttribute(name: string): string | null;
    setAttribute(name: string, value: string): void;
    setProperty(name: string, value: string): boolean;
    setInnerRML(rml: string): void;
    getInnerRML(): string;
    addClass(name: string): void;
    removeClass(name: string): void;
    addEventListener(type: string, fn: (ev: RmlEvent) => void): void;
}

declare const document: { getElementById(id: string): RmlElement | null };
declare const console:  { log(...args: unknown[]): void };