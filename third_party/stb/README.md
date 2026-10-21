# stb image codecs

Vendored from `nothings/stb` commit `a7d4c2175630e1ea5b6f8dd00ab96fed0b97b96d`; `stb_image` v2.30 and `stb_image_write` v1.16. The headers provide PNG/JPEG decode and encode and are distributed under the included MIT or public-domain license alternatives. No package-manager lookup or build-time network access is required.

Prism-Forge defines only PNG/JPEG codecs from these headers. The project has not independently audited this decoder; process only images from sources you trust and keep the pinned dependency updated as security fixes are reviewed. Prism-Forge applies compressed-file and decoded-pixel limits before full decode.
