import { createFileRoute } from "@tanstack/react-router";
import { CinderApp } from "@/components/cinder-app";

export const Route = createFileRoute("/")({
  component: CinderApp,
});
