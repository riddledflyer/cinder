import * as CheckboxPrimitive from "@radix-ui/react-checkbox";
import { Check } from "lucide-react";
import { cn } from "@/lib/cn";

export function Checkbox({
  className,
  ...props
}: CheckboxPrimitive.CheckboxProps) {
  return (
    <CheckboxPrimitive.Root
      className={cn(
        "flex size-5 shrink-0 items-center justify-center rounded-sm border border-border bg-bg text-accent-fg",
        "data-[state=checked]:border-accent data-[state=checked]:bg-accent",
        "disabled:opacity-40",
        className,
      )}
      {...props}
    >
      <CheckboxPrimitive.Indicator>
        <Check className="size-3.5" strokeWidth={2.5} />
      </CheckboxPrimitive.Indicator>
    </CheckboxPrimitive.Root>
  );
}
