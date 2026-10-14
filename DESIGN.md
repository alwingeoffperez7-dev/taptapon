# Design: TapTapon

World: **barangay segregation signage**, the stenciled labels painted on MRF drums and bins. A civic green shell, stencil numerals and tags, a working UI underneath. Stylesheet: `dashboard/css/styles.css`, shared by every page.

## Color (green palette)
| Token | Hex | Job |
|---|---|---|
| `--field` | `#0F4A33` | Civic green shell: header, points panel, admin stat strip |
| `--field-deep` / `--field-line` | `#0A3826` / `#2A6A4E` | Empty ticks, dividers on the field |
| `--on-field` / `--on-field-muted` | `#FFFFFF` / `#B9D6C4` | Text on the field |
| `--lime` | `#C4EA5A` | **Reserved for points earned** (the big number, filled ticks, counts, selection) |
| `--ground` | `#EEF2EA` | Pale green concrete page ground |
| `--paper` | `#FFFFFF` | Panels and inputs |
| `--ink` / `--muted` | `#0B2E21` / `#4A5F54` | Text |
| `--action` | `#1A7A4C` | **Only on things you can press**, plus "+1 earned" |
| `--amber` | `#E5A312` | Bin past the re-arm line (50%) |
| `--red` | `#B93A23` | Bin FULL (≥ 80%) |
| `--offline` | `#7C8A82` | Bin with no heartbeat for 2 min |

State is never color alone: FULL = hazard stripes + red ring + filled label; OFFLINE = dashed drum + dashed label; disabled buttons = dashed outline.

## Type
- **Big Shoulders Stencil Display 800**: numbers and painted tags only (points, %, stats, ranks, PET/RESIDUAL, OK/FULL/OFFLINE, wordmark). Never for sentences.
- **Atkinson Hyperlegible 400/700**: everything people read, chosen for legibility outdoors on a phone.
- Tabular numerals wherever numbers line up.

## Signature: drum gauge
Admin bin cards draw fill as a painted drum (70×128px, same scale for every bin) with dashed marks at `alertResetPercent` and `fullThresholdPercent`. The fill animates with `transform: scaleY` (no re-layout).

## Components
- Panels: white, 6px radius, soft offset shadow. Sections are separated by 2px painted rules, not nested cards.
- Buttons: 44px min height. `btn-action` (green), `btn-lime` (on the field), `btn-ghost` (on the field), `btn-text`.
- `.tag`: stencil label with a 2px outline, like paint on a drum.
- Focus: 3px amber outline; inputs get a green ring.

## Motion
One moment: when a student's points go up, the number is "stamped" on (scale + blur settle, 0.55s ease-out). Everything respects `prefers-reduced-motion`.

## Logo
`dashboard/img/logo.png` (the simulator reuses it). Until it exists, the stencil wordmark **TAP**<lime>**TAPON**</lime> shows.
