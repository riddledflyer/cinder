export type Media = "sd" | "nand";

export type Kind =
  | "game"
  | "demo"
  | "update"
  | "dlc"
  | "dsiware"
  | "homebrew"
  | "system";

export type Filter =
  | "all"
  | "game"
  | "update"
  | "dlc"
  | "dsiware"
  | "homebrew"
  | "system";

export type TitleEntry = {
  id: string;
  name: string;
  publisher: string;
  productCode: string;
  kind: Kind;
  media: Media;
  bytes: number;
  version: string;
  region: string;
};

export const CINDER_TITLE_ID = "0004000000C1D400";
export const STORAGE_KEY = "cinder-removed-v1";

const mb = (n: number) => Math.round(n * 1024 * 1024);

export const SEED: TitleEntry[] = [
  {
    id: "0004000000A11000",
    name: "Harbor Line",
    publisher: "Tideglass",
    productCode: "CTR-P-HARB",
    kind: "game",
    media: "sd",
    bytes: mb(1640),
    version: "1.0.0",
    region: "USA",
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
    region: "USA",
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
    region: "USA",
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
    region: "EUR",
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
    region: "EUR",
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
    region: "USA",
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
    region: "USA",
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
    region: "USA",
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
    region: "ALL",
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
    region: "USA",
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
    region: "EUR",
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
    region: "USA",
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
    region: "USA",
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
    region: "USA",
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
    region: "ALL",
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
    region: "ALL",
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
    region: "USA",
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
    region: "EUR",
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
    region: "USA",
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
    region: "USA",
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
    region: "USA",
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
    region: "ALL",
  },
];

export const FILTERS: { id: Filter; label: string }[] = [
  { id: "all", label: "All" },
  { id: "game", label: "Games" },
  { id: "update", label: "Updates" },
  { id: "dlc", label: "DLC" },
  { id: "dsiware", label: "NDS/DSi" },
  { id: "homebrew", label: "Homebrew" },
  { id: "system", label: "System" },
];

export type Shelf = "3ds" | "nds";

export function shelfOf(title: TitleEntry): Shelf | null {
  if (title.kind === "system" || title.id === CINDER_TITLE_ID) return null;
  if (title.kind === "dsiware") return "nds";
  return "3ds";
}

export function lowId(id: string): string {
  return id.slice(8).toUpperCase();
}

export function isDeletable(title: TitleEntry): boolean {
  if (title.kind === "system") return false;
  if (title.id === CINDER_TITLE_ID) return false;
  return true;
}

export function lockReason(title: TitleEntry): string | null {
  if (title.id === CINDER_TITLE_ID) return "This app cannot delete itself.";
  if (title.kind === "system") return "System software is locked. The console refuses these.";
  return null;
}

export function kindLabel(kind: Kind): string {
  switch (kind) {
    case "game":
      return "Game";
    case "demo":
      return "Demo";
    case "update":
      return "Update";
    case "dlc":
      return "DLC";
    case "dsiware":
      return "NDS";
    case "homebrew":
      return "Homebrew";
    case "system":
      return "System";
  }
}

export function kindMark(kind: Kind): string {
  switch (kind) {
    case "game":
      return "G";
    case "demo":
      return "M";
    case "update":
      return "U";
    case "dlc":
      return "D";
    case "dsiware":
      return "W";
    case "homebrew":
      return "H";
    case "system":
      return "S";
  }
}

export function matchesFilter(title: TitleEntry, filter: Filter): boolean {
  if (filter === "all") return true;
  if (filter === "game") return title.kind === "game" || title.kind === "demo";
  return title.kind === filter;
}

export function formatBytes(bytes: number): string {
  if (bytes < 1024) return `${bytes} B`;
  const units = ["KB", "MB", "GB"] as const;
  let value = bytes / 1024;
  let unit = 0;
  while (value >= 1024 && unit < units.length - 1) {
    value /= 1024;
    unit += 1;
  }
  const digits = value >= 10 || unit === 0 ? 1 : 2;
  const fixed = value.toFixed(digits);
  const pretty = fixed.endsWith(".0") ? fixed.slice(0, -2) : fixed;
  return `${pretty} ${units[unit]}`;
}

export function loadRemoved(): string[] {
  try {
    const raw = localStorage.getItem(STORAGE_KEY);
    if (!raw) return [];
    const parsed = JSON.parse(raw) as unknown;
    if (!Array.isArray(parsed)) return [];
    return parsed.filter((id): id is string => typeof id === "string");
  } catch {
    return [];
  }
}

export function saveRemoved(ids: string[]) {
  localStorage.setItem(STORAGE_KEY, JSON.stringify(ids));
}

export function relatedOf(title: TitleEntry, installed: TitleEntry[]): TitleEntry[] {
  if (title.kind !== "game" && title.kind !== "demo" && title.kind !== "homebrew") return [];
  const low = lowId(title.id);
  return installed.filter(
    (other) =>
      other.id !== title.id &&
      other.media === title.media &&
      lowId(other.id) === low &&
      (other.kind === "update" || other.kind === "dlc"),
  );
}

export function expandSelection(
  selected: TitleEntry[],
  installed: TitleEntry[],
  includeRelated: boolean,
): TitleEntry[] {
  const map = new Map(selected.filter(isDeletable).map((title) => [title.id, title]));
  if (includeRelated) {
    for (const title of selected) {
      if (!isDeletable(title)) continue;
      if (title.kind !== "game" && title.kind !== "homebrew" && title.kind !== "demo") continue;
      for (const other of relatedOf(title, installed)) {
        if (isDeletable(other)) map.set(other.id, other);
      }
    }
  }
  return [...map.values()];
}
