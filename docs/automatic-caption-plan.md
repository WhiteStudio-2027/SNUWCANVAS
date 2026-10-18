# Automatic caption integration

Implemented as an optional client for the independent caption-api service. The main image-generation server remains separate.

Flow: configure dedicated endpoint/key → list vision models → select image → generate once → review/edit result → apply to profile_caption → explicitly upload the same image. Requests have a 190-second client deadline and an explicit cancel action; there is no status polling. Caption API keys are masked and excluded from project exports and generated-image metadata. Closing the client cancels and joins its caption worker.

Future work: per-image batch captions and approved-caption history. Existing manual captions are not automatically replaced during reference uploads. Server-side shared GPU scheduling is not integrated; request cancellation does not guarantee instant Ollama inference termination.
