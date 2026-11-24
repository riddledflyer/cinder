import { i as __toESM } from "../_runtime.mjs";
import { _ as require_react, a as AlertDialogOverlay, g as require_jsx_runtime, i as AlertDialogDescription, n as AlertDialogCancel, o as AlertDialogPortal, r as AlertDialogContent, s as AlertDialogTitle, t as AlertDialog, u as Slot } from "../_libs/@radix-ui/react-alert-dialog+[...].mjs";
import { i as Check, n as Search, r as RotateCcw } from "../_libs/lucide-react.mjs";
import { n as clsx, t as cva } from "../_libs/class-variance-authority+clsx.mjs";
import { t as twMerge } from "../_libs/tailwind-merge.mjs";
import { n as CheckboxIndicator, t as Checkbox$1 } from "../_libs/@radix-ui/react-checkbox+[...].mjs";
//#region node_modules/.nitro/vite/services/ssr/assets/routes-DCX5lhz_.js
var import_react = /* @__PURE__ */ __toESM(require_react());
var import_jsx_runtime = require_jsx_runtime();
function cn(...inputs) {
	return twMerge(clsx(inputs));
}
var buttonVariants = cva("press inline-flex items-center justify-center gap-2 whitespace-nowrap rounded-md text-sm font-medium disabled:pointer-events-none disabled:opacity-40", {
	variants: {
		variant: {
			primary: "bg-accent text-accent-fg",
			ghost: "border border-border bg-transparent text-fg",
			danger: "bg-danger text-danger-fg",
			quiet: "bg-transparent text-muted"
		},
		size: {
			md: "h-11 px-4",
			sm: "h-10 px-3"
		}
	},
	defaultVariants: {
		variant: "ghost",
		size: "md"
	}
});
var Button = (0, import_react.forwardRef)(function Button({ className, variant, size, asChild, type = "button", ...props }, ref) {
	return /* @__PURE__ */ (0, import_jsx_runtime.jsx)(asChild ? Slot : "button", {
		ref,
		type,
		className: cn(buttonVariants({
			variant,
			size
		}), className),
		...props
	});
});
function Checkbox({ className, ...props }) {
	return /* @__PURE__ */ (0, import_jsx_runtime.jsx)(Checkbox$1, {
		className: cn("flex size-5 shrink-0 items-center justify-center rounded-sm border border-border bg-bg text-accent-fg", "data-[state=checked]:border-accent data-[state=checked]:bg-accent", "disabled:opacity-40", className),
		...props,
		children: /* @__PURE__ */ (0, import_jsx_runtime.jsx)(CheckboxIndicator, { children: /* @__PURE__ */ (0, import_jsx_runtime.jsx)(Check, {
			className: "size-3.5",
			strokeWidth: 2.5
		}) })
	});
}
var CINDER_TITLE_ID = "0004000000C1D400";
var STORAGE_KEY = "cinder-removed-v1";
var mb = (n) => Math.round(n * 1024 * 1024);
var SEED = [
	{
		id: "0004000000A11000",
		name: "Harbor Line",
		publisher: "Tideglass",
		productCode: "CTR-P-HARB",
		kind: "game",
		media: "sd",
		bytes: mb(1640),
		version: "1.0.0",
		region: "USA"
	},
	{
		id: "0004000E00A11000",
		name: "Harbor Line",
		publisher: "Tideglass",
		productCode: "CTR-U-HARB",
		kind: "update",
		media: "sd",
		bytes: mb(186),
		version: "1.4.0",
		region: "USA"
	},
	{
		id: "0004008C00A11000",
		name: "Harbor Line — Pier Lights",
		publisher: "Tideglass",
		productCode: "CTR-M-HARB",
		kind: "dlc",
		media: "sd",
		bytes: mb(64),
		version: "1.0.0",
		region: "USA"
	},
	{
		id: "0004000000A12000",
		name: "Paper Kite",
		publisher: "Northwind",
		productCode: "CTR-P-KITE",
		kind: "game",
		media: "sd",
		bytes: mb(820),
		version: "1.1.0",
		region: "EUR"
	},
	{
		id: "0004008C00A12000",
		name: "Paper Kite — Spare String",
		publisher: "Northwind",
		productCode: "CTR-M-KITE",
		kind: "dlc",
		media: "sd",
		bytes: mb(42),
		version: "1.0.0",
		region: "EUR"
	},
	{
		id: "0004000000A13000",
		name: "Moss Circuit",
		publisher: "Lowland",
		productCode: "CTR-P-MOSS",
		kind: "game",
		media: "sd",
		bytes: mb(2140),
		version: "1.0.0",
		region: "USA"
	},
	{
		id: "0004000E00A13000",
		name: "Moss Circuit",
		publisher: "Lowland",
		productCode: "CTR-U-MOSS",
		kind: "update",
		media: "sd",
		bytes: mb(410),
		version: "1.3.2",
		region: "USA"
	},
	{
		id: "0004008C00A13000",
		name: "Moss Circuit — Night Grid",
		publisher: "Lowland",
		productCode: "CTR-M-MOSS",
		kind: "dlc",
		media: "sd",
		bytes: mb(208),
		version: "1.0.0",
		region: "USA"
	},
	{
		id: "0004000000A14000",
		name: "Glass Orchard",
		publisher: "Seed & Co",
		productCode: "CTR-P-ORCH",
		kind: "game",
		media: "sd",
		bytes: mb(640),
		version: "1.0.0",
		region: "ALL"
	},
	{
		id: "0004000000A15000",
		name: "Night Ferry",
		publisher: "Lantern Row",
		productCode: "CTR-P-FERR",
		kind: "game",
		media: "sd",
		bytes: mb(1120),
		version: "1.2.0",
		region: "USA"
	},
	{
		id: "0004000000A16000",
		name: "Copper Archive",
		publisher: "Index Works",
		productCode: "CTR-P-COPP",
		kind: "game",
		media: "sd",
		bytes: mb(488),
		version: "1.0.1",
		region: "EUR"
	},
	{
		id: "0004000000A17000",
		name: "Salt Map",
		publisher: "Brine",
		productCode: "CTR-P-SALT",
		kind: "game",
		media: "sd",
		bytes: mb(1904),
		version: "1.0.0",
		region: "USA"
	},
	{
		id: "0004000E00A17000",
		name: "Salt Map",
		publisher: "Brine",
		productCode: "CTR-U-SALT",
		kind: "update",
		media: "sd",
		bytes: mb(96),
		version: "1.1.0",
		region: "USA"
	},
	{
		id: "0004000200A11000",
		name: "Harbor Line",
		publisher: "Tideglass",
		productCode: "CTR-P-HARX",
		kind: "demo",
		media: "sd",
		bytes: mb(96),
		version: "1.0.0",
		region: "USA"
	},
	{
		id: "0004000000B22000",
		name: "Field Notes",
		publisher: "Homebrew",
		productCode: "CTR-HB-NOTE",
		kind: "homebrew",
		media: "sd",
		bytes: mb(6),
		version: "2.0.0",
		region: "ALL"
	},
	{
		id: CINDER_TITLE_ID,
		name: "Cinder",
		publisher: "Riddle",
		productCode: "CTR-HB-CNDR",
		kind: "homebrew",
		media: "sd",
		bytes: mb(2),
		version: "3.6.0",
		region: "ALL"
	},
	{
		id: "000480044B4C4D00",
		name: "Pocket Loom",
		publisher: "Heddle",
		productCode: "TWL-P-LOOM",
		kind: "dsiware",
		media: "nand",
		bytes: mb(18),
		version: "1.0.0",
		region: "USA"
	},
	{
		id: "000480044B4E5400",
		name: "Minute Market",
		publisher: "Heddle",
		productCode: "TWL-P-MINT",
		kind: "dsiware",
		media: "nand",
		bytes: mb(12),
		version: "1.2.0",
		region: "EUR"
	},
	{
		id: "0004003000008F02",
		name: "Home Menu",
		publisher: "System",
		productCode: "CTR-N-HMME",
		kind: "system",
		media: "nand",
		bytes: mb(28),
		version: "11.17.0",
		region: "USA"
	},
	{
		id: "0004001000021000",
		name: "System Settings",
		publisher: "System",
		productCode: "CTR-N-SYST",
		kind: "system",
		media: "nand",
		bytes: mb(14),
		version: "11.17.0",
		region: "USA"
	},
	{
		id: "0004001000020100",
		name: "Download Play",
		publisher: "System",
		productCode: "CTR-N-DLP",
		kind: "system",
		media: "nand",
		bytes: mb(4),
		version: "11.4.0",
		region: "USA"
	},
	{
		id: "0004013000001C02",
		name: "Process Manager",
		publisher: "System",
		productCode: "CTR-N-PM",
		kind: "system",
		media: "nand",
		bytes: mb(1),
		version: "11.17.0",
		region: "ALL"
	}
];
function shelfOf(title) {
	if (title.kind === "system" || title.id === "0004000000C1D400") return null;
	if (title.kind === "dsiware") return "nds";
	return "3ds";
}
function lowId(id) {
	return id.slice(8).toUpperCase();
}
function isDeletable(title) {
	if (title.kind === "system") return false;
	if (title.id === "0004000000C1D400") return false;
	return true;
}
function lockReason(title) {
	if (title.id === "0004000000C1D400") return "This app cannot delete itself.";
	if (title.kind === "system") return "System software is locked. The console refuses these.";
	return null;
}
function kindLabel(kind) {
	switch (kind) {
		case "game": return "Game";
		case "demo": return "Demo";
		case "update": return "Update";
		case "dlc": return "DLC";
		case "dsiware": return "NDS";
		case "homebrew": return "Homebrew";
		case "system": return "System";
	}
}
function formatBytes(bytes) {
	if (bytes < 1024) return `${bytes} B`;
	const units = [
		"KB",
		"MB",
		"GB"
	];
	let value = bytes / 1024;
	let unit = 0;
	while (value >= 1024 && unit < units.length - 1) {
		value /= 1024;
		unit += 1;
	}
	const digits = value >= 10 || unit === 0 ? 1 : 2;
	const fixed = value.toFixed(digits);
	return `${fixed.endsWith(".0") ? fixed.slice(0, -2) : fixed} ${units[unit]}`;
}
function loadRemoved() {
	try {
		const raw = localStorage.getItem(STORAGE_KEY);
		if (!raw) return [];
		const parsed = JSON.parse(raw);
		if (!Array.isArray(parsed)) return [];
		return parsed.filter((id) => typeof id === "string");
	} catch {
		return [];
	}
}
function saveRemoved(ids) {
	localStorage.setItem(STORAGE_KEY, JSON.stringify(ids));
}
function relatedOf(title, installed) {
	if (title.kind !== "game" && title.kind !== "demo" && title.kind !== "homebrew") return [];
	const low = lowId(title.id);
	return installed.filter((other) => other.id !== title.id && other.media === title.media && lowId(other.id) === low && (other.kind === "update" || other.kind === "dlc"));
}
function expandSelection(selected, installed, includeRelated) {
	const map = new Map(selected.filter(isDeletable).map((title) => [title.id, title]));
	if (includeRelated) for (const title of selected) {
		if (!isDeletable(title)) continue;
		if (title.kind !== "game" && title.kind !== "homebrew" && title.kind !== "demo") continue;
		for (const other of relatedOf(title, installed)) if (isDeletable(other)) map.set(other.id, other);
	}
	return [...map.values()];
}
function matchesQuery(title, query) {
	const q = query.trim().toLowerCase();
	if (!q) return true;
	return title.name.toLowerCase().includes(q) || title.publisher.toLowerCase().includes(q) || title.productCode.toLowerCase().includes(q) || title.id.toLowerCase().includes(q);
}
var VERSION = "3.6";
function CinderApp() {
	const [removed, setRemoved] = (0, import_react.useState)([]);
	const [shelf, setShelf] = (0, import_react.useState)("3ds");
	const [query, setQuery] = (0, import_react.useState)("");
	const [selected, setSelected] = (0, import_react.useState)([]);
	const [cursorId, setCursorId] = (0, import_react.useState)(null);
	const [includeRelated, setIncludeRelated] = (0, import_react.useState)(true);
	const [freed, setFreed] = (0, import_react.useState)(0);
	const [dialog, setDialog] = (0, import_react.useState)("closed");
	const [pendingIds, setPendingIds] = (0, import_react.useState)([]);
	(0, import_react.useEffect)(() => {
		setRemoved(loadRemoved());
	}, []);
	const installed = (0, import_react.useMemo)(() => SEED.filter((title) => !removed.includes(title.id) && !pendingIds.includes(title.id)), [removed, pendingIds]);
	const visible = (0, import_react.useMemo)(() => installed.filter((title) => shelfOf(title) === shelf && matchesQuery(title, query)), [
		installed,
		shelf,
		query
	]);
	const cursor = visible.find((title) => title.id === cursorId) ?? visible[0] ?? null;
	const selectedTitles = installed.filter((title) => selected.includes(title.id));
	const selectedBytes = selectedTitles.reduce((sum, title) => sum + title.bytes, 0);
	const planned = expandSelection(selectedTitles, installed, includeRelated);
	const plannedBytes = planned.reduce((sum, title) => sum + title.bytes, 0);
	(0, import_react.useEffect)(() => {
		function onKey(event) {
			const target = event.target;
			if (target instanceof HTMLElement && (target.tagName === "INPUT" || target.tagName === "TEXTAREA")) return;
			if (dialog === "working") return;
			const key = event.key.toLowerCase();
			if (key === "arrowdown" || key === "arrowup") {
				event.preventDefault();
				if (visible.length === 0) return;
				const current = Math.max(0, visible.findIndex((title) => title.id === cursor?.id));
				const next = key === "arrowdown" ? Math.min(visible.length - 1, current + 1) : Math.max(0, current - 1);
				const id = visible[next]?.id;
				if (!id) return;
				setCursorId(id);
				document.getElementById(`title-${id}`)?.scrollIntoView({ block: "nearest" });
			} else if (key === "a" && cursor && isDeletable(cursor)) {
				event.preventDefault();
				toggle(cursor.id);
			} else if (key === "x") {
				event.preventDefault();
				const ids = visible.filter(isDeletable).map((title) => title.id);
				const allOn = ids.length > 0 && ids.every((id) => selected.includes(id));
				setSelected((current) => allOn ? current.filter((id) => !ids.includes(id)) : [.../* @__PURE__ */ new Set([...current, ...ids])]);
			} else if (key === "l") setShelf("3ds");
			else if (key === "r") setShelf("nds");
			else if (key === "enter" && planned.length > 0 && dialog === "closed") {
				event.preventDefault();
				setDialog("confirm");
			}
		}
		window.addEventListener("keydown", onKey);
		return () => window.removeEventListener("keydown", onKey);
	});
	function toggle(id) {
		const title = installed.find((item) => item.id === id);
		if (!title || !isDeletable(title)) return;
		setSelected((current) => current.includes(id) ? current.filter((item) => item !== id) : [...current, id]);
	}
	function rangeTo(index) {
		const start = visible.findIndex((title) => title.id === cursor?.id);
		if (start < 0) return;
		const [lo, hi] = start < index ? [start, index] : [index, start];
		const ids = visible.slice(lo, hi + 1).filter(isDeletable).map((title) => title.id);
		setSelected((current) => [.../* @__PURE__ */ new Set([...current, ...ids])]);
	}
	function switchShelf(next) {
		setShelf(next);
		setCursorId(null);
	}
	function runDelete() {
		const queue = expandSelection(selectedTitles, installed, includeRelated);
		if (queue.length === 0) return;
		setDialog("working");
		const reduce = window.matchMedia("(prefers-reduced-motion: reduce)").matches;
		let index = 0;
		const step = () => {
			if (index >= queue.length) {
				const ids = queue.map((title) => title.id);
				const bytes = queue.reduce((sum, title) => sum + title.bytes, 0);
				setRemoved((current) => {
					const next = [.../* @__PURE__ */ new Set([...current, ...ids])];
					saveRemoved(next);
					return next;
				});
				setSelected((current) => current.filter((id) => !ids.includes(id)));
				setFreed((current) => current + bytes);
				setPendingIds([]);
				setDialog("done");
				return;
			}
			setPendingIds(queue.slice(0, index + 1).map((title) => title.id));
			index += 1;
			window.setTimeout(step, reduce ? 0 : 90);
		};
		window.setTimeout(step, reduce ? 0 : 160);
	}
	function restoreLibrary() {
		setRemoved([]);
		saveRemoved([]);
		setSelected([]);
		setFreed(0);
		setPendingIds([]);
		setDialog("closed");
	}
	const shelfLabel = shelf === "nds" ? "NDS software" : "3DS software";
	const counts = {
		"3ds": installed.filter((title) => shelfOf(title) === "3ds").length,
		nds: installed.filter((title) => shelfOf(title) === "nds").length
	};
	const locked = cursor ? lockReason(cursor) : null;
	return /* @__PURE__ */ (0, import_jsx_runtime.jsxs)("main", {
		className: "min-h-dvh overflow-x-hidden bg-bg text-fg",
		children: [
			/* @__PURE__ */ (0, import_jsx_runtime.jsxs)("header", {
				className: "mx-auto flex max-w-6xl items-end justify-between gap-4 px-4 pt-6 pb-4",
				children: [/* @__PURE__ */ (0, import_jsx_runtime.jsxs)("div", {
					className: "flex items-center gap-3",
					children: [/* @__PURE__ */ (0, import_jsx_runtime.jsx)("img", {
						src: "/cinder-icon.png",
						alt: "",
						width: 56,
						height: 56,
						className: "size-14 rounded-xl"
					}), /* @__PURE__ */ (0, import_jsx_runtime.jsxs)("div", { children: [
						/* @__PURE__ */ (0, import_jsx_runtime.jsx)("p", {
							className: "text-xs font-medium text-muted",
							children: "Nintendo 3DS"
						}),
						/* @__PURE__ */ (0, import_jsx_runtime.jsx)("h1", {
							className: "text-2xl font-medium tracking-tight",
							children: "Cinder"
						}),
						/* @__PURE__ */ (0, import_jsx_runtime.jsxs)("p", {
							className: "text-xs text-muted",
							children: ["Version ", VERSION]
						})
					] })]
				}), /* @__PURE__ */ (0, import_jsx_runtime.jsxs)("div", {
					className: "flex shrink-0 gap-2",
					children: [/* @__PURE__ */ (0, import_jsx_runtime.jsxs)("a", {
						href: `/cinder-${VERSION}.cia`,
						download: `cinder-${VERSION}.cia`,
						className: "press rounded-md bg-accent px-3 py-2 text-sm text-accent-fg",
						children: [
							"cinder-",
							VERSION,
							".cia"
						]
					}), /* @__PURE__ */ (0, import_jsx_runtime.jsx)("a", {
						href: `/cinder-${VERSION}-source.zip`,
						download: `cinder-${VERSION}-source.zip`,
						className: "press rounded-md border border-border px-3 py-2 text-sm text-fg",
						children: "Source"
					})]
				})]
			}),
			/* @__PURE__ */ (0, import_jsx_runtime.jsxs)("div", {
				className: "mx-auto grid max-w-6xl items-start gap-4 px-4 pb-28 lg:grid-cols-[minmax(0,1fr)_19rem] lg:pb-10",
				children: [/* @__PURE__ */ (0, import_jsx_runtime.jsxs)("section", {
					className: "min-w-0 rounded-2xl border border-border bg-surface p-2",
					children: [/* @__PURE__ */ (0, import_jsx_runtime.jsxs)("div", {
						className: "rounded-xl bg-bg p-4 sm:p-5",
						children: [
							/* @__PURE__ */ (0, import_jsx_runtime.jsxs)("div", {
								className: "flex items-baseline justify-between gap-3",
								children: [/* @__PURE__ */ (0, import_jsx_runtime.jsxs)("p", {
									className: "text-xs text-muted",
									children: [shelfLabel, /* @__PURE__ */ (0, import_jsx_runtime.jsxs)("span", {
										className: "text-subtle",
										children: [" · v", VERSION]
									})]
								}), /* @__PURE__ */ (0, import_jsx_runtime.jsxs)("p", {
									className: "font-mono text-xs text-subtle tabular-nums",
									children: [
										selectedTitles.length,
										" selected · ",
										formatBytes(selectedBytes)
									]
								})]
							}),
							cursor ? /* @__PURE__ */ (0, import_jsx_runtime.jsxs)("div", {
								className: "mt-3 flex items-start gap-4",
								children: [/* @__PURE__ */ (0, import_jsx_runtime.jsx)(GameIcon, {
									title: cursor,
									large: true
								}), /* @__PURE__ */ (0, import_jsx_runtime.jsxs)("div", {
									className: "min-w-0 flex-1",
									children: [
										/* @__PURE__ */ (0, import_jsx_runtime.jsx)("h2", {
											className: "text-xl font-medium tracking-tight",
											children: cursor.name
										}),
										/* @__PURE__ */ (0, import_jsx_runtime.jsx)("p", {
											className: "text-sm text-muted",
											children: cursor.publisher
										}),
										/* @__PURE__ */ (0, import_jsx_runtime.jsxs)("dl", {
											className: "mt-3 grid grid-cols-2 gap-x-4 gap-y-3 text-sm sm:grid-cols-4",
											children: [
												/* @__PURE__ */ (0, import_jsx_runtime.jsxs)("div", { children: [/* @__PURE__ */ (0, import_jsx_runtime.jsx)("dt", {
													className: "text-xs text-subtle",
													children: "Version"
												}), /* @__PURE__ */ (0, import_jsx_runtime.jsxs)("dd", {
													className: "font-mono text-xs",
													children: ["v", cursor.version]
												})] }),
												/* @__PURE__ */ (0, import_jsx_runtime.jsxs)("div", { children: [/* @__PURE__ */ (0, import_jsx_runtime.jsx)("dt", {
													className: "text-xs text-subtle",
													children: "Size"
												}), /* @__PURE__ */ (0, import_jsx_runtime.jsx)("dd", {
													className: "tabular-nums",
													children: formatBytes(cursor.bytes)
												})] }),
												/* @__PURE__ */ (0, import_jsx_runtime.jsxs)("div", { children: [/* @__PURE__ */ (0, import_jsx_runtime.jsx)("dt", {
													className: "text-xs text-subtle",
													children: "Kind"
												}), /* @__PURE__ */ (0, import_jsx_runtime.jsx)("dd", { children: kindLabel(cursor.kind) })] }),
												/* @__PURE__ */ (0, import_jsx_runtime.jsxs)("div", { children: [/* @__PURE__ */ (0, import_jsx_runtime.jsx)("dt", {
													className: "text-xs text-subtle",
													children: "Media"
												}), /* @__PURE__ */ (0, import_jsx_runtime.jsx)("dd", { children: cursor.media === "sd" ? "SD card" : "NAND" })] }),
												/* @__PURE__ */ (0, import_jsx_runtime.jsxs)("div", { children: [/* @__PURE__ */ (0, import_jsx_runtime.jsx)("dt", {
													className: "text-xs text-subtle",
													children: "Product"
												}), /* @__PURE__ */ (0, import_jsx_runtime.jsx)("dd", {
													className: "font-mono text-xs",
													children: cursor.productCode
												})] }),
												/* @__PURE__ */ (0, import_jsx_runtime.jsxs)("div", { children: [/* @__PURE__ */ (0, import_jsx_runtime.jsx)("dt", {
													className: "text-xs text-subtle",
													children: "Title ID"
												}), /* @__PURE__ */ (0, import_jsx_runtime.jsx)("dd", {
													className: "font-mono text-xs break-all",
													children: cursor.id
												})] }),
												/* @__PURE__ */ (0, import_jsx_runtime.jsxs)("div", {
													className: "sm:col-span-2",
													children: [/* @__PURE__ */ (0, import_jsx_runtime.jsx)("dt", {
														className: "text-xs text-subtle",
														children: "Related"
													}), /* @__PURE__ */ (0, import_jsx_runtime.jsx)("dd", { children: relatedSummary(cursor, installed) })]
												})
											]
										}),
										locked ? /* @__PURE__ */ (0, import_jsx_runtime.jsx)("p", {
											className: "mt-3 text-xs text-muted",
											children: locked
										}) : null
									]
								})]
							}) : /* @__PURE__ */ (0, import_jsx_runtime.jsxs)("h2", {
								className: "mt-3 text-xl font-medium",
								children: ["Nothing in ", shelfLabel]
							}),
							/* @__PURE__ */ (0, import_jsx_runtime.jsxs)("p", {
								className: "mt-4 text-xs text-subtle tabular-nums",
								children: [
									"Freed this session ",
									formatBytes(freed),
									" · ",
									counts["3ds"] + counts.nds,
									" listed in the sample"
								]
							})
						]
					}), /* @__PURE__ */ (0, import_jsx_runtime.jsxs)("div", {
						className: "mt-2 rounded-xl bg-elevated p-3 sm:p-4",
						children: [
							/* @__PURE__ */ (0, import_jsx_runtime.jsxs)("div", {
								className: "flex flex-col gap-3 sm:flex-row sm:items-center",
								children: [/* @__PURE__ */ (0, import_jsx_runtime.jsxs)("div", {
									className: "flex rounded-md border border-border p-1",
									role: "tablist",
									"aria-label": "Library",
									children: [/* @__PURE__ */ (0, import_jsx_runtime.jsx)(ShelfButton, {
										current: shelf,
										value: "3ds",
										count: counts["3ds"],
										onSelect: switchShelf,
										label: "3DS"
									}), /* @__PURE__ */ (0, import_jsx_runtime.jsx)(ShelfButton, {
										current: shelf,
										value: "nds",
										count: counts.nds,
										onSelect: switchShelf,
										label: "NDS"
									})]
								}), /* @__PURE__ */ (0, import_jsx_runtime.jsxs)("label", {
									className: "flex h-10 min-w-0 flex-1 items-center gap-2 rounded-md border border-border bg-bg px-3",
									children: [/* @__PURE__ */ (0, import_jsx_runtime.jsx)(Search, {
										className: "size-4 shrink-0 text-subtle",
										"aria-hidden": "true"
									}), /* @__PURE__ */ (0, import_jsx_runtime.jsx)("input", {
										value: query,
										onChange: (event) => setQuery(event.target.value),
										placeholder: "Search name, code, or title ID",
										className: "min-w-0 flex-1 bg-transparent text-sm text-fg outline-none placeholder:text-subtle",
										"aria-label": "Search titles"
									})]
								})]
							}),
							/* @__PURE__ */ (0, import_jsx_runtime.jsx)("ul", {
								className: "mt-3 max-h-96 overflow-auto rounded-md border border-border",
								"aria-label": shelfLabel,
								children: visible.length === 0 ? /* @__PURE__ */ (0, import_jsx_runtime.jsx)("li", {
									className: "px-4 py-8 text-sm text-muted",
									children: shelf === "nds" ? "No NDS software in the sample." : "No 3DS software in the sample."
								}) : visible.map((title, index) => {
									const rowLocked = !isDeletable(title);
									const on = selected.includes(title.id);
									const focused = cursor?.id === title.id;
									return /* @__PURE__ */ (0, import_jsx_runtime.jsx)("li", {
										id: `title-${title.id}`,
										className: "border-b border-border last:border-b-0",
										children: /* @__PURE__ */ (0, import_jsx_runtime.jsxs)("label", {
											className: cn("flex min-h-14 items-center gap-3 px-3 py-2", focused && "bg-bg", rowLocked && "text-subtle"),
											onClick: (event) => {
												setCursorId(title.id);
												if (event.shiftKey) {
													event.preventDefault();
													rangeTo(index);
												}
											},
											children: [
												/* @__PURE__ */ (0, import_jsx_runtime.jsx)(Checkbox, {
													checked: on,
													disabled: rowLocked,
													onCheckedChange: () => toggle(title.id),
													"aria-label": `Select ${title.name}`
												}),
												/* @__PURE__ */ (0, import_jsx_runtime.jsx)(GameIcon, { title }),
												/* @__PURE__ */ (0, import_jsx_runtime.jsxs)("span", {
													className: "min-w-0 flex-1",
													children: [/* @__PURE__ */ (0, import_jsx_runtime.jsxs)("span", {
														className: "flex items-baseline justify-between gap-3",
														children: [/* @__PURE__ */ (0, import_jsx_runtime.jsx)("span", {
															className: "truncate text-sm",
															children: title.name
														}), /* @__PURE__ */ (0, import_jsx_runtime.jsx)("span", {
															className: "shrink-0 font-mono text-xs tabular-nums text-muted",
															children: formatBytes(title.bytes)
														})]
													}), /* @__PURE__ */ (0, import_jsx_runtime.jsxs)("span", {
														className: "mt-0.5 flex items-baseline justify-between gap-3 text-xs text-subtle",
														children: [/* @__PURE__ */ (0, import_jsx_runtime.jsx)("span", {
															className: "truncate",
															children: title.publisher
														}), /* @__PURE__ */ (0, import_jsx_runtime.jsxs)("span", {
															className: "shrink-0 font-mono",
															children: [
																"v",
																title.version,
																" · ",
																title.media === "sd" ? "SD" : "NAND"
															]
														})]
													})]
												})
											]
										})
									}, title.id);
								})
							}),
							/* @__PURE__ */ (0, import_jsx_runtime.jsx)("p", {
								className: "mt-3 text-xs text-subtle",
								children: "Keys: arrows move, A selects, X toggles select-all, L and R switch 3DS and NDS, Enter deletes. On a 3DS, the list is up immediately. Select opens install mode. Start+Select exits to the HOME Menu."
							})
						]
					})]
				}), /* @__PURE__ */ (0, import_jsx_runtime.jsxs)("aside", {
					className: "grid min-w-0 gap-4",
					children: [/* @__PURE__ */ (0, import_jsx_runtime.jsxs)("section", {
						className: "rounded-xl border border-border bg-surface p-4",
						children: [
							/* @__PURE__ */ (0, import_jsx_runtime.jsx)("h2", {
								className: "text-sm font-medium",
								children: "On a 3DS"
							}),
							/* @__PURE__ */ (0, import_jsx_runtime.jsxs)("p", {
								className: "mt-2 text-sm text-muted",
								children: [
									"This screen uses a sample library stored in the browser. Download cinder-",
									VERSION,
									".cia and install it with FBI or GodMode9. It lists real installed titles. System modules and applets stay hidden. Nintendo games and homebrew stay listed. Select switches to install mode for CIAs in sdmc:/cias/ and on the SD root. A file is installed when its size matches the CIA header."
								]
							}),
							/* @__PURE__ */ (0, import_jsx_runtime.jsxs)("ul", {
								className: "mt-3 list-disc space-y-2 pl-4 text-sm text-muted",
								children: [
									/* @__PURE__ */ (0, import_jsx_runtime.jsx)("li", { children: "Select opens install mode. Each CIA shows its icon, developer, version, file size, and install size." }),
									/* @__PURE__ */ (0, import_jsx_runtime.jsx)("li", { children: "Icons are the native 48×48 SMDH. NDS icons stay 32×32 and are filtered once. A real flipbook animates." }),
									/* @__PURE__ */ (0, import_jsx_runtime.jsxs)("li", { children: [
										"Install cinder-",
										VERSION,
										".cia with FBI. Title ID is 0004000000C1D400. It does not replace 2.5, so the old icon can be deleted from the list."
									] })
								]
							})
						]
					}), /* @__PURE__ */ (0, import_jsx_runtime.jsxs)("section", {
						className: "rounded-xl border border-border bg-surface p-4",
						children: [
							/* @__PURE__ */ (0, import_jsx_runtime.jsx)("h2", {
								className: "text-sm font-medium",
								children: "Delete"
							}),
							/* @__PURE__ */ (0, import_jsx_runtime.jsxs)("label", {
								className: "mt-3 flex items-start gap-3 text-sm text-muted",
								children: [/* @__PURE__ */ (0, import_jsx_runtime.jsx)(Checkbox, {
									checked: includeRelated,
									onCheckedChange: (value) => setIncludeRelated(value === true)
								}), /* @__PURE__ */ (0, import_jsx_runtime.jsx)("span", { children: "Also delete the update and DLC that belong to a selected game." })]
							}),
							/* @__PURE__ */ (0, import_jsx_runtime.jsxs)("p", {
								className: "mt-3 font-mono text-xs text-subtle tabular-nums",
								children: [
									planned.length,
									" queued · ",
									formatBytes(plannedBytes)
								]
							}),
							/* @__PURE__ */ (0, import_jsx_runtime.jsxs)("div", {
								className: "mt-3 flex flex-wrap gap-2",
								children: [/* @__PURE__ */ (0, import_jsx_runtime.jsx)(Button, {
									variant: "danger",
									disabled: planned.length === 0 || dialog === "working",
									onClick: () => setDialog("confirm"),
									children: "Delete"
								}), /* @__PURE__ */ (0, import_jsx_runtime.jsxs)(Button, {
									variant: "ghost",
									onClick: restoreLibrary,
									children: [/* @__PURE__ */ (0, import_jsx_runtime.jsx)(RotateCcw, {
										className: "size-4",
										"aria-hidden": "true"
									}), "Restore sample"]
								})]
							})
						]
					})]
				})]
			}),
			/* @__PURE__ */ (0, import_jsx_runtime.jsx)(AlertDialog, {
				open: dialog !== "closed",
				onOpenChange: (open) => {
					if (!open && dialog !== "working") setDialog("closed");
				},
				children: /* @__PURE__ */ (0, import_jsx_runtime.jsxs)(AlertDialogPortal, { children: [/* @__PURE__ */ (0, import_jsx_runtime.jsx)(AlertDialogOverlay, { className: "fixed inset-0 bg-bg/80" }), /* @__PURE__ */ (0, import_jsx_runtime.jsx)(AlertDialogContent, {
					className: "fixed top-1/2 left-1/2 w-full max-w-sm -translate-x-1/2 -translate-y-1/2 rounded-xl border border-border bg-surface p-4",
					children: dialog === "done" ? /* @__PURE__ */ (0, import_jsx_runtime.jsxs)(import_jsx_runtime.Fragment, { children: [
						/* @__PURE__ */ (0, import_jsx_runtime.jsx)(AlertDialogTitle, {
							className: "text-lg font-medium",
							children: "Removed"
						}),
						/* @__PURE__ */ (0, import_jsx_runtime.jsxs)(AlertDialogDescription, {
							className: "mt-2 text-sm text-muted",
							children: [
								"Freed this session ",
								formatBytes(freed),
								"."
							]
						}),
						/* @__PURE__ */ (0, import_jsx_runtime.jsx)("div", {
							className: "mt-4 flex justify-end",
							children: /* @__PURE__ */ (0, import_jsx_runtime.jsx)(AlertDialogCancel, {
								asChild: true,
								children: /* @__PURE__ */ (0, import_jsx_runtime.jsx)(Button, {
									variant: "primary",
									children: "Close"
								})
							})
						})
					] }) : /* @__PURE__ */ (0, import_jsx_runtime.jsxs)(import_jsx_runtime.Fragment, { children: [
						/* @__PURE__ */ (0, import_jsx_runtime.jsxs)(AlertDialogTitle, {
							className: "text-lg font-medium",
							children: [
								"Delete ",
								planned.length,
								" titles?"
							]
						}),
						/* @__PURE__ */ (0, import_jsx_runtime.jsxs)(AlertDialogDescription, {
							className: "mt-2 text-sm text-muted",
							children: [formatBytes(plannedBytes), " leaves the sample library. This does not touch a console."]
						}),
						/* @__PURE__ */ (0, import_jsx_runtime.jsx)("ul", {
							className: "mt-3 max-h-40 overflow-auto text-sm",
							children: planned.map((title) => /* @__PURE__ */ (0, import_jsx_runtime.jsxs)("li", {
								className: "flex items-center justify-between gap-3 py-1",
								children: [/* @__PURE__ */ (0, import_jsx_runtime.jsx)("span", {
									className: "truncate",
									children: title.name
								}), /* @__PURE__ */ (0, import_jsx_runtime.jsx)("span", {
									className: "shrink-0 text-xs text-subtle",
									children: kindLabel(title.kind)
								})]
							}, title.id))
						}),
						/* @__PURE__ */ (0, import_jsx_runtime.jsxs)("div", {
							className: "mt-4 flex justify-end gap-2",
							children: [/* @__PURE__ */ (0, import_jsx_runtime.jsx)(AlertDialogCancel, {
								asChild: true,
								children: /* @__PURE__ */ (0, import_jsx_runtime.jsx)(Button, {
									variant: "ghost",
									disabled: dialog === "working",
									children: "Cancel"
								})
							}), /* @__PURE__ */ (0, import_jsx_runtime.jsx)(Button, {
								variant: "danger",
								disabled: dialog === "working",
								onClick: runDelete,
								children: dialog === "working" ? "Deleting" : "Delete"
							})]
						})
					] })
				})] })
			})
		]
	});
}
function relatedSummary(title, installed) {
	const related = relatedOf(title, installed);
	if (related.length === 0) return "None";
	return related.map((item) => `${kindLabel(item.kind)}`).join(", ");
}
function ShelfButton({ current, value, count, onSelect, label }) {
	const active = current === value;
	return /* @__PURE__ */ (0, import_jsx_runtime.jsxs)("button", {
		type: "button",
		role: "tab",
		"aria-selected": active,
		onClick: () => onSelect(value),
		className: cn("press h-8 rounded-sm px-3 text-sm", active ? "bg-accent text-accent-fg" : "text-muted"),
		children: [label, /* @__PURE__ */ (0, import_jsx_runtime.jsx)("span", {
			className: "ml-2 tabular-nums",
			children: count
		})]
	});
}
var FILL = {
	accent: "fill-accent",
	danger: "fill-danger",
	ok: "fill-ok",
	fg: "fill-fg",
	muted: "fill-muted"
};
var STAMPS = {
	"0004000000A11000": [{
		ink: "accent",
		d: "M3 16h18v3H3zm5-1V7h8v8h-2V9h-4v6z"
	}, {
		ink: "danger",
		d: "M10 3h4v2h-4zm1 2h2v3h-2z"
	}],
	"0004000E00A11000": [{
		ink: "ok",
		d: "M12 3 19 12h-4v9H9v-9H5z"
	}],
	"0004008C00A11000": [{
		ink: "accent",
		d: "M3 18h18v2H3z"
	}, {
		ink: "danger",
		d: "M5 9h3v7H5zm5.5-4h3v11h-3zm5.5 5h3v6h-3z"
	}],
	"0004000200A11000": [{
		ink: "muted",
		d: "M6 3h2v18H6zm2 1h11l-3.5 4L19 12H8z"
	}],
	"0004000000A12000": [{
		ink: "accent",
		d: "M12 2 22 12 12 22 2 12z"
	}, {
		ink: "danger",
		d: "M12 8 16 12 12 16 8 12z"
	}],
	"0004008C00A12000": [{
		ink: "accent",
		d: "M4 8h16v3H4zm0 5h16v3H4z"
	}, {
		ink: "danger",
		d: "M8 5h3v14H8zm5 0h3v14h-3z"
	}],
	"0004000000A13000": [{
		ink: "accent",
		d: "M4 20V11h3v9zm6.5 0V6h3v14zM17 20v-8h3v8z"
	}, {
		ink: "ok",
		d: "M4 8h3v2H4zm6.5-4h3v2h-3z"
	}],
	"0004000E00A13000": [{
		ink: "ok",
		d: "M4 5h16v3H4zm0 5h12v3H4zm0 5h8v3H4z"
	}, {
		ink: "accent",
		d: "M16 14h4v5h-4z"
	}],
	"0004008C00A13000": [{
		ink: "accent",
		d: "M4 4h7v7H4zm9 0h7v7h-7zM4 13h7v7H4z"
	}, {
		ink: "danger",
		d: "M13 13h7v7h-7z"
	}],
	"0004000000A14000": [{
		ink: "ok",
		d: "M11 3h2v4h-2z"
	}, {
		ink: "accent",
		cx: 12,
		cy: 14,
		r: 6
	}],
	"0004000000A15000": [
		{
			ink: "accent",
			d: "M2 14h20l-3 6H5z"
		},
		{
			ink: "fg",
			d: "M11 4h2v10h-2z"
		},
		{
			ink: "danger",
			d: "M7 7h10v2H7z"
		}
	],
	"0004000000A16000": [
		{
			ink: "danger",
			d: "M3 10 12 3l9 7v2H3z"
		},
		{
			ink: "accent",
			d: "M5 12h14v9H5z"
		},
		{
			ink: "fg",
			d: "M8 15h3v2H8zm5 0h3v2h-3zM8 18h3v3H8z"
		}
	],
	"0004000000A17000": [{
		ink: "accent",
		d: "M11 2h2v20h-2zM2 11h20v2H2z"
	}, {
		ink: "danger",
		cx: 12,
		cy: 12,
		r: 3
	}],
	"0004000E00A17000": [{
		ink: "accent",
		d: "M4 5h9l7 7v9H4z"
	}, {
		ink: "ok",
		d: "M13 5v7h7z"
	}],
	"0004000000B22000": [{
		ink: "fg",
		d: "M6 3h12v18H6z"
	}, {
		ink: "accent",
		d: "M6 7h12v2H6zm0 4h12v2H6zm0 4h8v2H6z"
	}],
	"0004000000C1D400": [
		{
			ink: "danger",
			d: "M6 5h12a2 2 0 0 1 2 2v4H4V7a2 2 0 0 1 2-2z"
		},
		{
			ink: "accent",
			d: "M4 13h16v4a2 2 0 0 1-2 2H6a2 2 0 0 1-2-2z"
		},
		{
			ink: "fg",
			d: "M14 2h2v3h-2z"
		}
	],
	"000480044B4C4D00": [{
		ink: "accent",
		d: "M4 4h16v16H4z"
	}, {
		ink: "danger",
		d: "M4 11h16v2H4zM11 4h2v16h-2z"
	}],
	"000480044B4E5400": [
		{
			ink: "danger",
			d: "M3 11 12 4l9 7v2H3z"
		},
		{
			ink: "accent",
			d: "M6 13h12v7H6z"
		},
		{
			ink: "fg",
			d: "M11 15h2v5h-2z"
		}
	],
	"0004003000008F02": [{
		ink: "fg",
		d: "M4 4h16v16H4z"
	}, {
		ink: "accent",
		d: "M4 11h16v2H4zM11 11h2v9h-2z"
	}],
	"0004001000021000": [
		{
			ink: "fg",
			d: "M3 7h18v2H3zm0 8h18v2H3z"
		},
		{
			ink: "accent",
			cx: 8,
			cy: 8,
			r: 2.4
		},
		{
			ink: "danger",
			cx: 16,
			cy: 16,
			r: 2.4
		}
	],
	"0004001000020100": [{
		ink: "accent",
		d: "M11 3h2v9h-2zm-4 7h10l-5 6z"
	}, {
		ink: "danger",
		d: "M5 19h14v2H5z"
	}],
	"0004013000001C02": [
		{
			ink: "ok",
			cx: 7,
			cy: 12,
			r: 3.2
		},
		{
			ink: "accent",
			cx: 17,
			cy: 12,
			r: 3.2
		},
		{
			ink: "fg",
			d: "M10 11h4v2h-4z"
		}
	]
};
function GameIcon({ title, large = false }) {
	const shapes = STAMPS[title.id] ?? [{
		ink: "accent",
		d: "M4 4h16v16H4z"
	}, {
		ink: "danger",
		d: "M9 9h6v6H9z"
	}];
	return /* @__PURE__ */ (0, import_jsx_runtime.jsx)("span", {
		className: cn("grid shrink-0 place-items-center rounded-md border border-border bg-bg", large ? "size-16" : "size-10"),
		"aria-hidden": "true",
		children: /* @__PURE__ */ (0, import_jsx_runtime.jsx)("svg", {
			viewBox: "0 0 24 24",
			className: large ? "size-11" : "size-7",
			children: shapes.map((shape, index) => shape.d ? /* @__PURE__ */ (0, import_jsx_runtime.jsx)("path", {
				d: shape.d,
				className: FILL[shape.ink]
			}, index) : /* @__PURE__ */ (0, import_jsx_runtime.jsx)("circle", {
				cx: shape.cx,
				cy: shape.cy,
				r: shape.r,
				className: FILL[shape.ink]
			}, index))
		})
	});
}
var SplitComponent = CinderApp;
//#endregion
export { SplitComponent as component };
