import { Slot } from "@radix-ui/react-slot";
import { cva, type VariantProps } from "class-variance-authority";
import { forwardRef, type ButtonHTMLAttributes } from "react";
import { cn } from "@/lib/cn";

const buttonVariants = cva(
  "press inline-flex items-center justify-center gap-2 whitespace-nowrap rounded-md text-sm font-medium disabled:pointer-events-none disabled:opacity-40",
  {
    variants: {
      variant: {
        primary: "bg-accent text-accent-fg",
        ghost: "border border-border bg-transparent text-fg",
        danger: "bg-danger text-danger-fg",
        quiet: "bg-transparent text-muted",
      },
      size: {
        md: "h-11 px-4",
        sm: "h-10 px-3",
      },
    },
    defaultVariants: { variant: "ghost", size: "md" },
  },
);

type ButtonProps = ButtonHTMLAttributes<HTMLButtonElement> &
  VariantProps<typeof buttonVariants> & { asChild?: boolean };

export const Button = forwardRef<HTMLButtonElement, ButtonProps>(function Button(
  { className, variant, size, asChild, type = "button", ...props },
  ref,
) {
  const Comp = asChild ? Slot : "button";
  return (
    <Comp ref={ref} type={type} className={cn(buttonVariants({ variant, size }), className)} {...props} />
  );
});
