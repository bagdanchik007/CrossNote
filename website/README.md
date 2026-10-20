# CrossNote website

This is a dependency-free static project page. It describes the current prototype and deliberately does not advertise installers or online sync that are not available yet.

## Preview locally

From the repository root, run:

```sh
python -m http.server 8000 --directory website
```

Then open <http://localhost:8000>. The German page is the default; use the `EN` link for the English page.

Project and documentation links point to the repository's public GitHub pages and therefore work when only the `website/` directory is hosted as a static site.

## Publishing

The pages are plain HTML and CSS and can be hosted by any static file host. Configure a hosting target explicitly before enabling a publishing workflow; this repository does not publish the site automatically.
